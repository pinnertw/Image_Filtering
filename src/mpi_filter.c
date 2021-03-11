#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h> 

#include "gif_lib.h"
#include "basic_structure.h"

// TODO rankx not local root
// TODO rankx with local root
// TODO FILTER distribution
// TODO Re export
void
mpi_filter_rank_0_case1(int ** p, int* width_all, int* height_all, int n_images, int world_size, int start_image_index)
{
    int i, j;
    MPI_Status sta;

    // We first deal with each image with 1 process
    int height, width;
    MPI_Scatter(height_all+start_image_index, 1, MPI_INT, &height, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(width_all+start_image_index, 1, MPI_INT, &width, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    // Send image to every nodes
    for (i=1; i<world_size; i++){
        j = i + start_image_index;
        MPI_Send(p[j], width_all[j] * height_all[j], MPI_INT, i, 1, MPI_COMM_WORLD);
    }
    int* p_local;
    p_local = p[0 + start_image_index];
    classic_blur_filter_per_image(p_local, 5, 20, width, height);
    classic_sobel_filter_per_image(p_local, width, height);

    // Get image processed from nodes
    for (i=1; i<world_size; i++){
        j = i + start_image_index;
        MPI_Recv(p[j], width_all[j] * height_all[j], MPI_INT, i, 1, MPI_COMM_WORLD, &sta);
    }
}

void
mpi_filter_other_rank_case1()
{
    MPI_Status sta;
    int height, width;
    MPI_Scatter(NULL, 1, MPI_INT, &height, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(NULL, 1, MPI_INT, &width, 1,  MPI_INT, 0, MPI_COMM_WORLD);
    int* p_local;
    p_local = (int*) malloc(height * width * sizeof(int));
    MPI_Recv(p_local, height*width, MPI_INT, 0, 1, MPI_COMM_WORLD, &sta);
    // Processing
    classic_blur_filter_per_image(p_local, 5, 20, width, height);
    classic_sobel_filter_per_image(p_local, width, height);
    // Send back the image.
    MPI_Send(p_local, height*width, MPI_INT, 0, 1, MPI_COMM_WORLD);
}

// TODO process image with the groop image_comm
void
mpi_filter_case2_local_root(int* p_local, int height, int width, MPI_Comm image_comm)
{
}

// TODO process image with the groop image_comm
void
mpi_filter_case2_local_process()
{
}

void
mpi_filter_rank_0_case2(int ** p, int * width_all, int* height_all, int n_images, int start_image_index)
{
    int i, j;
    MPI_Status sta;

    // Send n_images to determine their group
    n_images -= start_image_index;
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
    printf("n_images : %d, start_image_index : %d\n", n_images, start_image_index);
    // Send image to every roots
    for (i=1; i < n_images; i++){
        j = i + start_image_index;
        MPI_Send(p[j], width_all[j] * height_all[j], MPI_INT, i, 2, roots);
    }
    int* p_local;
    p_local = p[0 + start_image_index];

    mpi_filter_case2_local_root(p_local, height, width, image_comm);

    // Get image processed from roots
    for (i=1; i < n_images; i++){
        j = i + start_image_index;
        MPI_Recv(p[j], width_all[j] * height_all[j], MPI_INT, i, 2, roots, &sta);
    }

    printf("Group %d rank %d !\n", 0, local_rank);
}


void
mpi_filter_other_rank_case2(int world_rank)
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

        mpi_filter_case2_local_root(p_local, height, width, image_comm);

        MPI_Send(p_local, width * height, MPI_INT, 0, 2, roots);
    }
    else
    {
        mpi_filter_case2_local_process();
    }
    // Get image processed from roots
    printf("Group %d rank %d !\n", world_rank % n_images, local_rank);
}


int mpi_filter_rank_0(animated_gif * image)
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
        mpi_filter_rank_0_case1(p, width_all, height_all, n_images, world_size, start_image_index);
    }

    /* ############## Second case : need ghost shell ############### */
    dealing_case = 2;
    if (start_image_index < n_images)
    {
        MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
        mpi_filter_rank_0_case2(p, width_all, height_all, n_images, start_image_index);
    }
    /* ############## END ############### */
    dealing_case = 0;
    MPI_Bcast( &dealing_case, 1, MPI_INT, 0, MPI_COMM_WORLD);
    printf("Primary Done \n");
}


int mpi_filter_other_rank(world_rank)
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
            mpi_filter_other_rank_case1();
        }
        /* ############## Second case : need ghost shell ############### */
        if (dealing_case == 2)
        {
            mpi_filter_other_rank_case2(world_rank);
        }
    }
    printf("Local Done \n");
}

int mpi_filter(int argc, char ** argv)
{
    int world_rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0){
        char * input_filename;
        char * output_filename;
        struct timeval t1, t2;
        double duration;
        animated_gif* image;
        if (argc < 3)
        {
            fprintf(stderr, "Usage: %s input.gif output.gif \n", argv[0]);
            return 1;
        }
        input_filename = argv[1];
        output_filename = argv[2];

        ////////////////////////////IMPORT///////////////////////////
        /* IMPORT Timer start */
        gettimeofday(&t1, NULL);

        /* Load file and store the pixels in array */
        image = load_pixels(input_filename);
        if (image == NULL) {return 1;}

        /* IMPORT Timer stop */
        gettimeofday(&t2, NULL);

        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "GIF loaded from file %s with %d image(s) in %lf s\n",
                input_filename, image->n_images, duration);

        ////////////////////////////FILTER///////////////////////////
        /* Blur + Sobel filter Timer start */
        gettimeofday(&t1, NULL);

        mpi_filter_rank_0(image);

        /* Blur + Sobel filter Timer stop */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "SOBEL done in %lf s", duration);

        ////////////////////////////EXPORT///////////////////////////
        /* EXPORT Timer start */
        gettimeofday(&t1, NULL);

        if (!store_pixels(output_filename, image)){return 1;}

        /* EXPORT Timer stop */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec-t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr, "Export done in %lf s in file %s\n", duration, output_filename);
    }
    else{
        mpi_filter_other_rank(world_rank);
    }
    MPI_Finalize();
    return 0;
}
