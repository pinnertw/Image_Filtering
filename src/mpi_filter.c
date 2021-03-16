#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#include "gif_lib.h"
#include "basic_structure.h"

void
mpi_filter_rank_0_case1(int ** p, int* width_all, int* height_all, int n_images, int world_size, int start_image_index, int method)
{
    int i, j;
    MPI_Request req[2*world_size];
    MPI_Status sta[2*world_size];

    // We first deal with each image with 1 process
    int height, width;
    MPI_Scatter(height_all+start_image_index, 1, MPI_INT, &height, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(width_all+start_image_index, 1, MPI_INT, &width, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    // Send image to every nodes
    for (i=1; i<world_size; i++){
        j = i + start_image_index;
        MPI_Isend(p[j], width_all[j] * height_all[j], MPI_INT, i, 1, MPI_COMM_WORLD, req+i-1);
    }
    int* p_local;
    p_local = p[0 + start_image_index];

    if (method == 0)
    {
#if MPI_DEBUG
        printf("Using classic functions\n");
#endif
        classic_blur_filter_per_image(p_local, 5, 20, width, height);
        classic_sobel_filter_per_image(p_local, width, height);
    }
    else if (method == 1)
    {
#if MPI_DEBUG
        printf("Using openmp functions\n");
#endif
        openmp_blur_filter_per_image(p_local, 5, 20, width, height);
        openmp_sobel_filter_per_image(p_local, width, height);
    }
    else if (method == 2)
    {
#if MPI_DEBUG
        printf("Using cuda functions\n");
#endif
        cuda_filter_per_image(p_local, 5, 20, width, height);
    }

    // Get image processed from nodes
    for (i=1; i<world_size; i++){
        j = i + start_image_index;
        MPI_Irecv(p[j], width_all[j] * height_all[j], MPI_INT, i, 1, MPI_COMM_WORLD, req+i+world_size - 2);
    }
    MPI_Waitall(2*(world_size-1), req, sta);
}

void
mpi_filter_other_rank_case1(int method)
{
    MPI_Status sta;
    int height, width;
    MPI_Scatter(NULL, 1, MPI_INT, &height, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(NULL, 1, MPI_INT, &width, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    int* p_local;
    p_local = (int*) malloc(height * width * sizeof(int));
    MPI_Recv(p_local, height*width, MPI_INT, 0, 1, MPI_COMM_WORLD, &sta);
    // Processing
    if (method == 0)
    {
#if MPI_DEBUG
        printf("Using classic functions\n");
#endif
        classic_blur_filter_per_image(p_local, 5, 20, width, height);
        classic_sobel_filter_per_image(p_local, width, height);
    }
    else if (method == 1)
    {
#if MPI_DEBUG
        printf("Using openmp functions\n");
#endif
        openmp_blur_filter_per_image(p_local, 5, 20, width, height);
        openmp_sobel_filter_per_image(p_local, width, height);
    }
    else if (method == 2)
    {
#if MPI_DEBUG
        printf("Using cuda functions\n");
#endif
        cuda_filter_per_image(p_local, 5, 20, width, height);
    }
    // Send back the image.
    MPI_Send(p_local, height*width, MPI_INT, 0, 1, MPI_COMM_WORLD);
}

void 
mpi_update_image(int* p, int width, int height, int left, int right, int size_exchange, MPI_Comm image_comm, int rank)
{
    if (size_exchange > width) return exit(1);
    // p of size height * (left+width+right)
    MPI_Request req[4];
    MPI_Status sta[4];

    // Left/Right column type
    MPI_Datatype column;
    MPI_Type_vector(height, size_exchange, left+width+right, MPI_INT, &column);
    MPI_Type_commit(&column);

    // Get/Send left/right
    int messages_transferred = 0;
    if (left != 0) // Send left to left, receive left's right
    {
        MPI_Isend(p+left, 1, column, rank-1, 2, image_comm, req+messages_transferred);
        messages_transferred++;
        MPI_Irecv(p+left-size_exchange, 1, column, rank-1, 2, image_comm, req+messages_transferred);
        messages_transferred++;
    }
    if (right != 0) // send right to right, receive right's left
    {
        MPI_Isend(p+left+width-size_exchange, 1, column, rank+1, 2, image_comm, req+messages_transferred);
        messages_transferred++;
        MPI_Irecv(p+left+width, 1, column, rank+1, 2, image_comm, req+messages_transferred);
        messages_transferred++;
    }

    MPI_Waitall(messages_transferred, req, sta);
}

void
mpi_blur_filter_per_image(int* p, int size, int threshold, int width, int height, int left, int right, MPI_Comm image_comm, int method)
{
    int rank;
    MPI_Comm_rank(image_comm, &rank);
    int end = 0;
    int n_iter = 0;
    int j, k;

    /* Allocate array of new pixels */
    int width_total = left+width+right;
    int * new;
    new = (int *)malloc(width_total*height*sizeof( int )) ;

    /* Perform at least one blur iteration */
    do
    {
        mpi_update_image(p, width, height, left, right, size, image_comm, rank);
        n_iter++ ;

        if (method == 0)
        {
#if MPI_DEBUG
            printf("Using classic functions\n");
#endif
            end = classic_blur_in_while(p, new, size, threshold, width_total, height);
        }
        else if (method == 1)
        {
#if MPI_DEBUG
            printf("Using openmp functions for blur, %d th\n", n_iter);
#endif
            end = openmp_blur_in_while(p, new, size, threshold, width_total, height);
        }
        else if (method == 2)
        {
            end = cuda_blur_in_while(p, size, threshold, width_total, height);
        }
        MPI_Allreduce(&end, &end, 1, MPI_INT, MPI_PROD, image_comm);
    }
    while ( threshold > 0 && !end ) ;

#if MPI_DEBUG
    fprintf(stderr, "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif
    free (new);
}

void
mpi_sobel_filter_per_image(int* p, int width, int height, int left, int right, MPI_Comm image_comm, int method)
{
    int rank;
    MPI_Comm_rank(image_comm, &rank);
    mpi_update_image(p, width, height, left, right, 1, image_comm, rank);
    if (method == 0)
    {
#if MPI_DEBUG
        printf("Using classic functions\n");
#endif
        classic_sobel_filter_per_image(p, left+width+right, height);
    }
    else if (method == 1)
    {
#if MPI_DEBUG
        printf("Using openmp functions\n");
#endif
        openmp_sobel_filter_per_image(p, left+width+right, height);
    }
    else if (method == 2)
    {
#if MPI_DEBUG
        printf("Using cuda functions\n");
#endif
        cuda_sobel_filter_per_image(p, left+width+right, height);
    }
}

void
mpi_filter_case2_local_root(int* p, int width_global, int height, MPI_Comm image_comm, int method)
{
    int i, j;
    
    int world_size;
    MPI_Comm_size(image_comm, &world_size);

    MPI_Request req[2*world_size];
    MPI_Status sta[2*world_size];

    // For local root : define height/width, left/right for everyone
    int* width_all, *width_cum_sum, *left_all, *right_all;
    int width, left, right;
    int rest = width_global % world_size;
    width_all = (int*) malloc(world_size * sizeof(int));
    width_cum_sum = (int*) malloc((world_size+1) * sizeof(int));
    left_all = (int*) malloc(world_size * sizeof(int));
    right_all = (int*) malloc(world_size * sizeof(int));
    width_cum_sum[0] = 0;
    for(i = 0; i < world_size; i++)
    {
        width_all[i] = width_global / world_size;
        left_all[i] = 5;
        right_all[i] = 5;
        if (i < rest)
        {
            width_all[i]++;
        }
        width_cum_sum[i+1] = width_cum_sum[i] + width_all[i];
    }
    left_all[0] = 0;
    right_all[world_size-1] = 0;
#if MPI_DEBUG
    fprintf(stderr, "Sending characteristic from local root\n");
#endif

    // Send these values
    MPI_Bcast( &height, 1, MPI_INT, 0, image_comm);
    MPI_Scatter(width_all, 1, MPI_INT, &width, 1,  MPI_INT, 0, image_comm);
    MPI_Scatter(left_all, 1, MPI_INT, &left, 1,  MPI_INT, 0, image_comm);
    MPI_Scatter(right_all, 1, MPI_INT, &right, 1,  MPI_INT, 0, image_comm);

    // Send image to every nodes
    // For the first n_images % world_size images, we get one more width
    MPI_Datatype column_one_more;
    MPI_Type_vector(height, width, width_global, MPI_INT, &column_one_more);
    MPI_Type_commit(&column_one_more);

    // For the last images, we get normal width
    MPI_Datatype column;
    MPI_Type_vector(height, width-1, width_global, MPI_INT, &column);
    MPI_Type_commit(&column);
#if MPI_DEBUG
    fprintf(stderr, "%d process get(s) one more column\n", rest);
#endif
    // Send !!
    if (rest == 0)
    {
        for (i=1; i<world_size; i++){
            MPI_Isend(p+width_cum_sum[i], 1, column_one_more, i, 2, image_comm, req+i-1);
        }
    }
    else
    {
        for (i=1; i<rest; i++){
            MPI_Isend(p+width_cum_sum[i], 1, column_one_more, i, 2, image_comm, req+i-1);
        }
        for (i=rest; i<world_size; i++){
            if(rest != 0)
            {
            MPI_Isend(p+width_cum_sum[i], 1, column, i, 2, image_comm, req+i-1);
            }
        }
    }

    int* p_local;
    p_local = (int*) malloc(height * (left+width+right) * sizeof(int));
    int local_width = left+width+right;
    for (i=left; i<width+left; i++){
        for (j=0; j<height; j++){
            p_local[CONV(j,i,local_width)] = p[CONV(j,i,width_global)];
        }
    }

#if MPI_DEBUG
    fprintf(stderr, "MPI_local image starts processing\n") ;
#endif
#if time_eval_filters
    struct timeval t3, t4;
    double duration2;
    gettimeofday(&t3, NULL);
#endif

    // Processing
    mpi_blur_filter_per_image(p_local, 5, 20, width, height, left, right, image_comm, method);

#if time_eval_filters
    gettimeofday(&t4, NULL);
    duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Blur filter done in %lf s\n", duration2);
    printf("%lf ", duration2);
    gettimeofday(&t3, NULL);
#endif

    mpi_sobel_filter_per_image(p_local, width, height, left, right, image_comm, method);

#if time_eval_filters
    gettimeofday(&t4, NULL);
    duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Sobel filter done in %lf s\n", duration2) ;
    printf("%lf ", duration2);
#endif

    // Get image processed from nodes
    if (rest == 0)
    {
        for (i=1; i<world_size; i++){
            MPI_Irecv(p+width_cum_sum[i], 1, column_one_more, i, 2, image_comm, req+i+world_size-2);
        }
    }
    else
    {
        for (i=1; i<rest; i++){
            MPI_Irecv(p+width_cum_sum[i], 1, column_one_more, i, 2, image_comm, req+i+world_size-2);
        }
        for (i=rest; i<world_size; i++){
            MPI_Irecv(p+width_cum_sum[i], 1, column, i, 2, image_comm, req+i+world_size-2);
        }
    }
    for (i=left; i<width+left; i++){
        for (j=0; j<height; j++){
            p[CONV(j,i,width_global)] = p_local[CONV(j,i,local_width)];
        }
    }

    MPI_Waitall(2*(world_size-1), req, sta);
    // Finalize
    MPI_Type_free(&column_one_more);
    MPI_Type_free(&column);
}


void
mpi_filter_case2_local_process(MPI_Comm image_comm, int method)
{
    MPI_Status sta;
    
    // Get height, width, left/right
    int height, width, left, right;
    MPI_Bcast( &height, 1, MPI_INT, 0, image_comm);
    MPI_Scatter(NULL, 1, MPI_INT, &width, 1,  MPI_INT, 0, image_comm);
    MPI_Scatter(NULL, 1, MPI_INT, &left, 1,  MPI_INT, 0, image_comm);
    MPI_Scatter(NULL, 1, MPI_INT, &right, 1,  MPI_INT, 0, image_comm);

    int* p_local;
    p_local = (int*) malloc(height * (left+width+right) * sizeof(int));
    // Receive message
    MPI_Datatype column;
    MPI_Type_vector(height, width, left+width+right, MPI_INT, &column);
    MPI_Type_commit(&column);
    MPI_Recv(p_local+left, 1, column, 0, 2, image_comm, &sta);

    // Processing
    mpi_blur_filter_per_image(p_local, 5, 20, width, height, left, right, image_comm, method);
    mpi_sobel_filter_per_image(p_local, width, height, left, right, image_comm, method);

    // Send matrix back
    MPI_Send(p_local+left, 1, column, 0, 2, image_comm);

    // Finalize
    MPI_Type_free(&column);
}

void
mpi_filter_rank_0_case2(int ** p, int * width_all, int* height_all, int n_images, int start_image_index, int method)
{
    int i, j;
    n_images -= start_image_index;
    MPI_Request req[2*n_images];
    MPI_Status sta[2*n_images];

    // Send n_images to determine their group
    MPI_Bcast( &n_images, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Split comm group
    MPI_Comm image_comm; // local image processing group
    MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &image_comm);

    MPI_Comm roots; // Group with all the roots
    MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &roots);
    // Get local rank
    int local_rank;
    MPI_Comm_rank(image_comm, &local_rank);


    // Send size to every roots
    int height, width;
    MPI_Scatter(height_all+start_image_index, 1, MPI_INT, &height, 1,  MPI_INT, 0, roots);
    MPI_Scatter(width_all+start_image_index, 1, MPI_INT, &width, 1,  MPI_INT, 0, roots);

#if MPI_DEBUG
    fprintf(stderr, "n_images : %d, start_image_index : %d\n", n_images, start_image_index);
#endif

    // Send image to every roots
    for (i=1; i < n_images; i++){
        j = i + start_image_index;
        MPI_Isend(p[j], width_all[j] * height_all[j], MPI_INT, i, 2, roots, req+i-1);
    }
    int* p_local;
    p_local = p[0 + start_image_index];

#if MPI_DEBUG
    fprintf(stderr, "All images sent, start processing\n");
#endif
    mpi_filter_case2_local_root(p_local, width, height, image_comm, method);

    // Get image processed from roots
    for (i=1; i < n_images; i++){
        j = i + start_image_index;
        MPI_Irecv(p[j], width_all[j] * height_all[j], MPI_INT, i, 2, roots, req+i+n_images-2);    }

#if MPI_DEBUG
    fprintf(stderr, "All images received, end processing\n");
    fprintf(stderr, "Group %d rank %d !\n", 0, local_rank);
#endif
    MPI_Waitall(2*(n_images-1), req, sta);
}


void
mpi_filter_other_rank_case2(int world_rank, int method)
{
    MPI_Status sta;

    // Get n_images to determine the group
    int n_images;
    MPI_Bcast( &n_images, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Split comm group
    MPI_Comm image_comm; // Group for image processing
    MPI_Comm_split(MPI_COMM_WORLD, world_rank % n_images, world_rank, &image_comm);

    MPI_Comm roots; // Group with all the roots
    MPI_Comm_split(MPI_COMM_WORLD, world_rank / n_images, world_rank, &roots);
    bool local_root = ((world_rank / n_images) == 0);
    // Get local rank
    int local_rank;
    MPI_Comm_rank(image_comm, &local_rank);

    if (local_root)
    {
        int height, width;
        MPI_Scatter(NULL, 1, MPI_INT, &height, 1,  MPI_INT, 0, roots);
        MPI_Scatter(NULL, 1, MPI_INT, &width, 1,  MPI_INT, 0, roots);
        int * p_local;
        p_local = (int*) malloc(height * width * sizeof(int));
        MPI_Recv(p_local, width * height, MPI_INT, 0, 2, roots, &sta);

        mpi_filter_case2_local_root(p_local, width, height, image_comm, method);

        MPI_Send(p_local, width * height, MPI_INT, 0, 2, roots);
    }
    else
    {
        mpi_filter_case2_local_process(image_comm, method);
    }
    // Get image processed from roots
#if MPI_DEBUG
    fprintf(stderr, "Group %d rank %d !\n", world_rank % n_images, local_rank);
#endif
}


int mpi_filter_rank_0(animated_gif * image, int method)
{
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int n_images = image->n_images;
    int ** p = image->p;
    int* width_all = image->width;
    int* height_all = image->height;

    int dealing_case, start_image_index;
    /* ############# First case : size < n_image ########### */
    dealing_case = 1;
    for (start_image_index = 0; start_image_index < n_images - world_size; start_image_index += world_size){
        MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
        mpi_filter_rank_0_case1(p, width_all, height_all, n_images, world_size, start_image_index, method);
    }

    /* ############## Second case : need ghost shell ############### */
    dealing_case = 2;
    if (start_image_index < n_images)
    {
        MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
        mpi_filter_rank_0_case2(p, width_all, height_all, n_images, start_image_index, method);
    }
    /* ############## END ############### */
    dealing_case = 0;
    MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
#if MPI_DEBUG
    fprintf(stderr, "Primary Done \n");
#endif
}

int mpi_filter_other_rank(int world_rank, int method)
{
    int i;
    int dealing_case = 1;

    while (dealing_case != 0)
    {
        MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
        /* ############# First case : size < n_image ########### */
        // We first deal with each image with 1 process
        if (dealing_case == 1)
        {
            mpi_filter_other_rank_case1(method);
        }
        /* ############## Second case : need ghost shell ############### */
        if (dealing_case == 2)
        {
            mpi_filter_other_rank_case2(world_rank, method);
        }
    }
#if MPI_DEBUG
    fprintf(stderr, "Local Done \n");
#endif
}

int mpi_filter(int argc, char ** argv, int method)
{
    int world_rank, world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0){
#if SOBELF_DEBUG
        fprintf(stderr, "\nUsing MPI functions \n");
#endif

        char * input_filename;
        char * output_filename;
        animated_gif* image;

        input_filename = argv[1];
        output_filename = argv[2];

        ////////////////////////////IMPORT///////////////////////////
#if time_eval
        /* IMPORT Timer start */
        struct timeval t1, t2;
        double duration;
        gettimeofday(&t1, NULL);
#endif

        /* Load file and store the pixels in array */
        image = load_pixels(input_filename);
        if (image == NULL) {return 1;}

#if time_eval
        /* IMPORT Timer stop */
        gettimeofday(&t2, NULL);

        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "GIF loaded from file %s with %d image(s) in %lf s\n",
                input_filename, image->n_images, duration);
        printf("%s %lf ", input_filename, duration);

        ////////////////////////////FILTER///////////////////////////
        if (method == 0) printf("MPI-%d_classic ", world_size);
        else if (method == 1) printf("MPI-%d_OpenMP ", world_size);
        else if (method == 2) printf("MPI-%d_Cuda ", world_size);
        /* Blur + Sobel filter Timer start */
        gettimeofday(&t1, NULL);
#endif

#if time_eval_both
        struct timeval t3, t4;
        double duration2;
        gettimeofday(&t3, NULL);
#endif

        mpi_filter_rank_0(image, method);

#if time_eval_both
        /* Blur + Sobel filter Timer stop */
        gettimeofday(&t4, NULL);
        duration2 = (t4.tv_sec-t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
        fprintf(stderr, "SOBEL done in %lf s\n", duration2);
        printf("%lf \n", duration2);
#endif
#if time_eval
        /* Blur + Sobel filter Timer stop */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "SOBEL done in %lf s\n", duration);
        printf("%lf ", duration);

        ////////////////////////////EXPORT///////////////////////////
        /* EXPORT Timer start */
        gettimeofday(&t1, NULL);
#endif

        if (!store_pixels(output_filename, image)){return 1;}

#if time_eval
        /* EXPORT Timer stop */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "Export done in %lf s in file %s\n", duration, output_filename);
        printf("%lf \n", duration);
#endif
    }
    else{
        mpi_filter_other_rank(world_rank, method);
    }
    MPI_Finalize();
    return 0;
}
