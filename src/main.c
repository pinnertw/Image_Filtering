/*
 * INF560
 * BRANCH INCLUDING OPEN MP - FROM FINAL (7th) VERSION MPI
 * reste à faire: traiter cas où nb_process < nb_images
 * Image Filtering Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h> 

#include "gif_lib.h"

/* Set this macro to 1 to enable debugging information */
#define SOBELF_DEBUG 0

/* Represent one pixel from the image */
typedef struct pixel
{
    int r ; /* Red */
    int g ; /* Green */
    int b ; /* Blue */
} pixel ;

/* Represent one GIF image (animated or not */
typedef struct animated_gif
{
    int n_images ; /* Number of images */
    int * width ; /* Width of each image */
    int * height ; /* Height of each image */
    pixel ** p ; /* Pixels of each image */
    GifFileType * g ; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif ;

#define CONV(l,c,nb_c) \
    (l)*(nb_c)+(c)

/*
 * Load a GIF image from a file and return a
 * structure of type animated_gif.
 */
int *
load_pixels_mpi( GifFileType * g, int width, int height,int n_images ){
    ColorMapObject * colmap ;
    int error ;
    int i,j,k;
    int c ;
    MPI_Comm_rank(MPI_COMM_WORLD, &i);
    i =i%n_images;

    /* Get the global colormap */
    colmap = g->SColorMap ;
    if ( colmap == NULL ) 
    {
        fprintf( stderr, "Error global colormap is NULL\n" ) ;
        return NULL ;
    }

    /* Allocate the array of pixels to be returned */
    int * local_picture = (int *)malloc( width*height*sizeof( int) ) ;

    /* Traverse the image and fill pixels */
    for ( j = 0 ; j < height ; j++ ) 
    {
        for (k=0;k<width;k++){
            

            c = g->SavedImages[i].RasterBits[j*width+k] ;

            local_picture[k*height+j] = (colmap->Colors[c].Red + colmap->Colors[c].Green +colmap->Colors[c].Blue)/3;
        }
    }

    return local_picture ;
}

int 
output_modified_read_gif( char * filename, GifFileType * g ) 
{
    GifFileType * g2 ;
    int error2 ;

#if SOBELF_DEBUG
    printf( "Starting output to file %s\n", filename ) ;
#endif

    g2 = EGifOpenFileName( filename, false, &error2 ) ;
    if ( g2 == NULL )
    {
        fprintf( stderr, "Error EGifOpenFileName %s\n",
                filename ) ;
        return 0 ;
    }

    g2->SWidth = g->SWidth ;
    g2->SHeight = g->SHeight ;
    g2->SColorResolution = g->SColorResolution ;
    g2->SBackGroundColor = g->SBackGroundColor ;
    g2->AspectByte = g->AspectByte ;
    g2->SColorMap = g->SColorMap ;
    g2->ImageCount = g->ImageCount ;
    g2->SavedImages = g->SavedImages ;
    g2->ExtensionBlockCount = g->ExtensionBlockCount ;
    g2->ExtensionBlocks = g->ExtensionBlocks ;

    error2 = EGifSpew( g2 ) ;
    if ( error2 != GIF_OK ) 
    {
        fprintf( stderr, "Error after writing g2: %d <%s>\n", 
                error2, GifErrorString(g2->Error) ) ;
        return 0 ;
    }

    return 1 ;
}


int
store_pixels( char * filename, int * buf, int n_images,int width,int height,GifFileType * g)
{
    int n_colors = 0 ;
    int i, j, k,l ;
    GifColorType * colormap ;

    /* Initialize the new set of colors */
    colormap = (GifColorType *)malloc( 256 * sizeof( GifColorType ) ) ;
    if ( colormap == NULL ) 
    {
        fprintf( stderr,
                "Unable to allocate 256 colors\n" ) ;
        return 0 ;
    }

    /* Everything is white by default */
    for ( i = 0 ; i < 256 ; i++ ) 
    {
        colormap[i].Red = 255 ;
        colormap[i].Green = 255 ;
        colormap[i].Blue = 255 ;
    }

    /* Change the background color and store it */
    int moy ;
    moy = (
            g->SColorMap->Colors[ g->SBackGroundColor ].Red
            +
            g->SColorMap->Colors[ g->SBackGroundColor ].Green
            +
            g->SColorMap->Colors[ g->SBackGroundColor ].Blue
            )/3 ;
    if ( moy < 0 ) moy = 0 ;
    if ( moy > 255 ) moy = 255 ;

#if SOBELF_DEBUG
    printf( "[DEBUG] Background color (%d,%d,%d) -> (%d,%d,%d)\n",
            g->SColorMap->Colors[ image->g->SBackGroundColor ].Red,
            g->SColorMap->Colors[ image->g->SBackGroundColor ].Green,
            g->SColorMap->Colors[ image->g->SBackGroundColor ].Blue,
            moy, moy, moy ) ;
#endif

    colormap[0].Red = moy ;
    colormap[0].Green = moy ;
    colormap[0].Blue = moy ;

    g->SBackGroundColor = 0 ;

    n_colors++ ;

    /* Process extension blocks in main structure */
    for ( j = 0 ; j < g->ExtensionBlockCount ; j++ )
    {
        int f ;

        f = g->ExtensionBlocks[j].Function ;
        if ( f == GRAPHICS_EXT_FUNC_CODE )
        {
            int tr_color = g->ExtensionBlocks[j].Bytes[3] ;

            if ( tr_color >= 0 &&
                    tr_color < 255 )
            {

                int found = -1 ;

                moy = 
                    (
                     g->SColorMap->Colors[ tr_color ].Red
                     +
                     g->SColorMap->Colors[ tr_color ].Green
                     +
                     g->SColorMap->Colors[ tr_color ].Blue
                    ) / 3 ;
                if ( moy < 0 ) moy = 0 ;
                if ( moy > 255 ) moy = 255 ;

#if SOBELF_DEBUG
                printf( "[DEBUG] Transparency color image %d (%d,%d,%d) -> (%d,%d,%d)\n",
                        i,
                        g->SColorMap->Colors[ tr_color ].Red,
                        g->SColorMap->Colors[ tr_color ].Green,
                        g->SColorMap->Colors[ tr_color ].Blue,
                        moy, moy, moy ) ;
#endif

                for ( k = 0 ; k < n_colors ; k++ )
                {
                    if ( 
                            moy == colormap[k].Red
                            &&
                            moy == colormap[k].Green
                            &&
                            moy == colormap[k].Blue
                       )
                    {
                        found = k ;
                    }
                }
                if ( found == -1  ) 
                {
                    if ( n_colors >= 256 ) 
                    {
                        fprintf( stderr, 
                                "Error: Found too many colors inside the image\n"
                               ) ;
                        return 0 ;
                    }

#if SOBELF_DEBUG
                    printf( "[DEBUG]\tNew color %d\n",
                            n_colors ) ;
#endif

                    colormap[n_colors].Red = moy ;
                    colormap[n_colors].Green = moy ;
                    colormap[n_colors].Blue = moy ;


                    g->ExtensionBlocks[j].Bytes[3] = n_colors ;

                    n_colors++ ;
                } else
                {
#if SOBELF_DEBUG
                    printf( "[DEBUG]\tFound existing color %d\n",
                            found ) ;
#endif
                    g->ExtensionBlocks[j].Bytes[3] = found ;
                }
            }
        }
    }
    
    int f ,tr_color,found;

    for ( i = 0 ; i < n_images ; i++ )
    {
        for ( j = 0 ; j < g->SavedImages[i].ExtensionBlockCount ; j++ )
        {

            f = g->SavedImages[i].ExtensionBlocks[j].Function ;
            if ( f == GRAPHICS_EXT_FUNC_CODE )
            {
                tr_color = g->SavedImages[i].ExtensionBlocks[j].Bytes[3] ;

                if ( tr_color >= 0 &&
                        tr_color < 255 )
                {

                    found = -1 ;

                    moy = 
                        (
                         g->SColorMap->Colors[ tr_color ].Red
                         +
                         g->SColorMap->Colors[ tr_color ].Green
                         +
                         g->SColorMap->Colors[ tr_color ].Blue
                        ) / 3 ;
                    if ( moy < 0 ) moy = 0 ;
                    if ( moy > 255 ) moy = 255 ;

#if SOBELF_DEBUG
                    printf( "[DEBUG] Transparency color image %d (%d,%d,%d) -> (%d,%d,%d)\n",
                            i,
                            g->SColorMap->Colors[ tr_color ].Red,
                            g->SColorMap->Colors[ tr_color ].Green,
                            g->SColorMap->Colors[ tr_color ].Blue,
                            moy, moy, moy ) ;
#endif

                    for ( k = 0 ; k < n_colors ; k++ )
                    {
                        if ( 
                                moy == colormap[k].Red
                                &&
                                moy == colormap[k].Green
                                &&
                                moy == colormap[k].Blue
                           )
                        {
                            found = k ;
                        }
                    }
                    if ( found == -1  ) 
                    {
                        
                        if ( n_colors >= 256 ) 
                        {
                            fprintf( stderr, 
                                    "Error: Found too many colors inside the image\n"
                                   ) ;
                            //return 0 ;
                        }
                        

#if SOBELF_DEBUG
                        printf( "[DEBUG]\tNew color %d\n",
                                n_colors ) ;
#endif

                        colormap[n_colors].Red = moy ;
                        colormap[n_colors].Green = moy ;
                        colormap[n_colors].Blue = moy ;


                        g->SavedImages[i].ExtensionBlocks[j].Bytes[3] = n_colors ;

                        n_colors++ ;
                    } else
                    {
#if SOBELF_DEBUG
                        printf( "[DEBUG]\tFound existing color %d\n",
                                found ) ;
#endif
                        g->SavedImages[i].ExtensionBlocks[j].Bytes[3] = found ;
                    }
                }
            }
        }
    }

#if SOBELF_DEBUG
    printf( "[DEBUG] Number of colors after background and transparency: %d\n",
            n_colors ) ;
#endif

    /* Find the number of colors inside the image */
    for ( i = 0 ; i < n_images ; i++ )
    {

#if SOBELF_DEBUG
        printf( "OUTPUT: Processing image %d (total of %d images) -> %d x %d\n",
                i, n_images, width, height ) ;
#endif

        for ( j = 0 ; j < width * height ; j++ ) 
        {
            int found = 0 ;
            for ( k = 0 ; k < n_colors ; k++ )
            {
                if ( buf[i*width*height+j] == colormap[k].Red &&
                        buf[i*width*height+j] == colormap[k].Green &&
                        buf[i*width*height+j] == colormap[k].Blue )
                {
                    found = 1 ;
                }
            }

            if ( found == 0 ) 
            {
                if ( n_colors >= 256 ) 
                {
                    fprintf( stderr, 
                            "Error: Found too many colors inside the image\n"
                           ) ;
                    return 0 ;
                }

#if SOBELF_DEBUG
                printf( "[DEBUG] Found new %d color (%d,%d,%d)\n",
                        n_colors, p[i][j].r, p[i][j].g, p[i][j].b ) ;
#endif

                colormap[n_colors].Red = buf[i*width*height+j];
                colormap[n_colors].Green = buf[i*width*height+j] ;
                colormap[n_colors].Blue = buf[i*width*height+j];
                n_colors++ ;
            }
        }
    }

#if SOBELF_DEBUG
    printf( "OUTPUT: found %d color(s)\n", n_colors ) ;
#endif


    /* Round up to a power of 2 */
    if ( n_colors != (1 << GifBitSize(n_colors) ) )
    {
        n_colors = (1 << GifBitSize(n_colors) ) ;
    }

#if SOBELF_DEBUG
    printf( "OUTPUT: Rounding up to %d color(s)\n", n_colors ) ;
#endif

    /* Change the color map inside the animated gif */
    ColorMapObject * cmo ;

    cmo = GifMakeMapObject( n_colors, colormap ) ;
    if ( cmo == NULL )
    {
        fprintf( stderr, "Error while creating a ColorMapObject w/ %d color(s)\n",
                n_colors ) ;
        return 0 ;
    }

    g->SColorMap = cmo ;

    /* Update the raster bits according to color map */
    for ( i = 0 ; i < n_images ; i++ )
    {
        for ( j = 0 ; j < height ; j++ ) 
        {
            for (l=0;l<width;l++){

                int found_index = -1 ;
                for ( k = 0 ; k < n_colors ; k++ ) 
                {
                    if ( buf[i*width*height+l*height+j] == g->SColorMap->Colors[k].Red &&
                            buf[i*width*height+l*height+j] == g->SColorMap->Colors[k].Green &&
                            buf[i*width*height+l*height+j] == g->SColorMap->Colors[k].Blue )
                    {
                        found_index = k ;
                    }
                }

                if ( found_index == -1 ) 
                {
                    fprintf( stderr,
                            "Error: Unable to find a pixel in the color map\n" ) ;
                    return 0 ;
                }

                g->SavedImages[i].RasterBits[j*width+l] = found_index ;
            }
            
        }
    }


    /* Write the final image */
    if ( !output_modified_read_gif( filename, g ) ) { return 0 ; }

    return 1 ;
}


int apply_blur_filter_mpi(int * sub_local_picture, int size, int threshold, int height,int width){

    int j, k ;
    int * new ;
    int max_diff_local=0;

    /* Allocate array of new pixels */
    new = (int *)malloc(width * height * sizeof( int ) ) ;

//#pragma omp parallel for schedule(static,height*width/10) private(k) //improves performance
    for(j=1; j<height-1; j++)
    {
        for(k=1; k<width-1; k++)
        {
            new[CONV(k,j,height)] = sub_local_picture[CONV(k,j,height)] ;
        }
    }

    /* Apply blur on top part of image (10%) */
    int end=height/10-size;
    int t,stencil_j, stencil_k;
#pragma omp parallel for schedule(static,height*width/10) private(k,stencil_j,stencil_k,t) 
    for(j=size; j<end; j++)
    {
        for(k=size; k<width-size; k++)
        {
            t = 0 ;

            for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
            {
                for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                {
                    t += sub_local_picture[CONV(k+stencil_k,j+stencil_j,height)] ;
                }
            }

            new[CONV(k,j,height)] = t / ( (2*size+1)*(2*size+1) ) ;
        }
    }

    int start=height*0.9+size;
    /* Apply blur on the bottom part of the image (10%) */
#pragma omp parallel for schedule(static,height*width/10) private(k,stencil_j,stencil_k,t) 
    for(j=start; j<height-size; j++)
    {
        for(k=size; k<width-size; k++)
        {
            int stencil_j, stencil_k ;
            int t = 0 ;

            for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
            {
                for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                {
                    t += sub_local_picture[CONV(k+stencil_k,j+stencil_j,height)] ;
                }
            }

            new[CONV(k,j,height)] = t / ( (2*size+1)*(2*size+1) ) ;
        }
    }

float diff;
//#pragma omp parallel for schedule(dynamic,5) private(k,diff) //worsens performance
    for(j=1; j<height-1; j++)
    {
        for(k=1; k<width-1; k++)
        {

            

            diff = abs(new[CONV(k  ,j  ,height)] - sub_local_picture[CONV(k  ,j  ,height)]) ;

            if ( diff > max_diff_local  ) {
                max_diff_local = diff ;
            }

            sub_local_picture[CONV(k  ,j  ,height)] = new[CONV(k  ,j ,height)] ;
        }
    }

    free (new) ;
    return max_diff_local;

}


void apply_sobel_filter_mpi(int * sub_local_picture,int height,int width){
    //width=nb of columns to process
    //sub_local_picture includes "width" columns surrounded by the left and right columns-> width+2 columns in total
    int j, k ;

    int * sobel ;

    sobel = (int *)malloc(width * height * sizeof( int ) ) ;


#pragma omp parallel for schedule(static,30) private(k) //improves performance
    for(j=1; j<height-1; j++)
    {
        for(k=1; k<width-1; k++)
        {
            int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
            int pixel_blue_so, pixel_blue_s, pixel_blue_se;
            int pixel_blue_o , pixel_blue  , pixel_blue_e ;
            float deltaX_blue ;
            float deltaY_blue ;
            float val_blue;

            pixel_blue    = sub_local_picture[CONV(k  ,j  ,height)] ;
            pixel_blue_n  = sub_local_picture[CONV(k  ,j-1,height)] ;
            pixel_blue_s  = sub_local_picture[CONV(k ,j+1,height)] ;
            pixel_blue_no = sub_local_picture[CONV(k-1,j-1,height)] ;
            pixel_blue_o  = sub_local_picture[CONV(k-1,j  ,height)] ;
            pixel_blue_so = sub_local_picture[CONV(k-1,j+1,height)]  ;
            pixel_blue_ne = sub_local_picture[CONV(k+1,j-1,height)] ;
            pixel_blue_e = sub_local_picture[CONV(k+1,j ,height)] ;
            pixel_blue_se = sub_local_picture[CONV(k+1,j+1,height)] ;
                
            
            

            deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

            deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

            val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;


            if ( val_blue > 50 ) 
            {
                //white
                sobel[CONV(k  ,j  ,height)] = 255 ;
            } else
            {
                //black
                sobel[CONV(k ,j  ,height)] = 0 ;
            }
            
        }
    }

#pragma omp parallel for schedule(static) private(k) //decreases performance
    for(j=1; j<height-1; j++)
    {
        for(k=1; k<width-1; k++)
        {
            sub_local_picture[CONV(k ,j  ,height)] = sobel[CONV(k  ,j  ,height)] ;
        }
    }

    free (sobel) ;
    
}

/*
 * Main entry point
 */
int 
main( int argc, char ** argv )
{
    char * input_filename ; 
    char * output_filename ;
    int * local_picture; // picture of the gif on which the process works (already greyed); has meaning only when local_rank=0
    int * buf; //toutes les images grisées bout à bout
    struct timeval t1, t2,t3;
    double duration_sobel,duration_blur ;
    int world_root=0;
    int local_root=0;
    int world_rank;
    int local_rank;
    int local_size; //nb of processes working on this picture
    int width = 0;
    int height = 0;
    GifFileType * g ;
    int n_images;
    int error;
    MPI_Comm image_comm; //communicator between processes working on a particular image of the GIF
    int * sub_local_picture; // has (process_nb_col rows+stencil) rows if first or last, (process_nb_col rows+2*stencil) rows otherwise
    int process_nb_col; //number of columns attributed to the process
    int process_nb_pixels; //(width+stencil)*process_nb_rows pour les bords, (width+2*stencil)*process_nb_rows pour les autres
    int * szbuf,*displs; 
    int i,k; // for loops
    MPI_Request * req;
    MPI_Status * sta;
    MPI_Request req2;
    MPI_Status sta2;
    int stencil=5;
    int threshold=20;
    MPI_Request req3[4];
    MPI_Status sta3[4];
    int end=0;
    int max_diff_local;
    int end_local;
    int * end_array;

    /* Check command-line arguments */
    if ( argc < 3 )
    {
        fprintf( stderr, "Usage: %s input.gif output.gif \n", argv[0] ) ;
        return 1 ;
    }

    input_filename = argv[1] ;
    output_filename = argv[2] ;

    MPI_Init(&argc, &argv); /* MPI Initialization */
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    //MPI_Comm_size(MPI_COMM_WORLD, &size);


    /* Open the GIF image (read mode) */
    g = DGifOpenFileName( input_filename, &error ) ;
    if ( g == NULL ) {
        fprintf( stderr, "Error DGifOpenFileName %s\n", input_filename ) ;
    }


    /* Read the GIF image */
    error = DGifSlurp( g ) ;
    if ( error != GIF_OK )
    {
        fprintf( stderr, 
                "Error DGifSlurp: %d <%s>\n", error, GifErrorString(g->Error) ) ;
        g = NULL ;
    }

    /* Grab the number of images and the size of images */
    n_images = g->ImageCount ;

    width = g->SavedImages[0].ImageDesc.Width ;
    
    height = g->SavedImages[0].ImageDesc.Height ;

    
    MPI_Comm_split( MPI_COMM_WORLD, world_rank%n_images, world_rank, &image_comm); 
    MPI_Comm_rank(image_comm, &local_rank);
    MPI_Comm_size(image_comm, &local_size);

    process_nb_col = width/local_size; 
    if (local_rank<width%local_size) {
        process_nb_col++;
    }


    process_nb_pixels = process_nb_col*height;

    if (local_rank==local_root){


        local_picture = load_pixels_mpi(g,width,height,n_images) ;
        szbuf = (int*)malloc(local_size*sizeof(int));
        displs = (int*)malloc((local_size+1)*sizeof(int));
        end_array = (int*)malloc(local_size*sizeof(int));
    }
    else {
        local_picture = NULL;
        szbuf = displs = NULL;
        end_array=NULL;
    }

    if (local_size!=1 &&(local_rank == local_root || local_rank==local_size-1)) {
        process_nb_pixels+=height*stencil;
    }

    else if (local_size!=1){
        process_nb_pixels+=height*2*stencil;
    }

    if (world_rank==world_root){
        /* IMPORT Timer start */
        gettimeofday(&t1, NULL);
    }

    MPI_Gather(&process_nb_pixels, 1, MPI_INT,szbuf, 1, MPI_INT, local_root, image_comm);

    if (local_rank == local_root) {
        displs[0] = 0;
        for(k = 0 ; k< local_size ; k++ ) {
            displs[k+1] = displs[k] + szbuf[k]-2*stencil*height;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    sub_local_picture = (int*)malloc(process_nb_pixels*sizeof(int));
    MPI_Scatterv(local_picture, szbuf, displs,MPI_INT,sub_local_picture, process_nb_pixels, MPI_INT, local_root, image_comm);
    

    MPI_Barrier(MPI_COMM_WORLD);

    /* Apply blur filter with convergence value (still to be modified)*/
    
    
    
    do{
        end_local=1;

        if ((local_rank>0)&&(local_rank<local_size-1)){
            max_diff_local=apply_blur_filter_mpi(sub_local_picture,stencil,threshold,height,process_nb_col+2*stencil); 
        }
        else if (local_size==1){
            max_diff_local=apply_blur_filter_mpi(sub_local_picture,stencil,threshold,height,process_nb_col); 
        }
        else if (local_rank==0){
            max_diff_local=apply_blur_filter_mpi(sub_local_picture,stencil,threshold,height,process_nb_col+stencil); 
        }
        else{
            max_diff_local=apply_blur_filter_mpi(sub_local_picture,stencil,threshold,height,process_nb_col+stencil); 
        }

        if (max_diff_local>threshold){
            end_local=0;
        }


        //check if threshold has been reached somewhere and update neighbors (if local_size>1)
        if (local_size>1){

            MPI_Gather(&end_local,1,MPI_INT,end_array,1,MPI_INT,local_root,image_comm);
            if (local_rank == local_root) {
                for(k = 1; k< local_size ; k++ ) {
                    end_local = end_local*end_array[k];
                }
                for(k = 0; k< local_size ; k++ ) {
                    end_array[k] = end_local; //we end the loop only if all differences are below the threshold
                }
            }
            MPI_Scatter(end_array, 1, MPI_INT,&end_local, 1, MPI_INT, local_root,image_comm);



            if (local_rank==local_root){
                req3[0]=MPI_REQUEST_NULL;
                req3[2]=MPI_REQUEST_NULL;
                //tag 0 for left received, tag 1 for right received
                MPI_Isend(sub_local_picture+process_nb_pixels-2*stencil*height, height*stencil, MPI_INT, local_root+1, 0, image_comm, req3+1); 
                MPI_Irecv(sub_local_picture+process_nb_pixels-stencil*height, height*stencil, MPI_INT, local_root+1, 1, image_comm, req3+3); 
            }
            else if (local_rank==local_size-1){
                req3[1]=MPI_REQUEST_NULL;
                req3[3]=MPI_REQUEST_NULL;
                MPI_Isend(sub_local_picture+height*stencil, height*stencil, MPI_INT, local_size-2, 1, image_comm, req3); 
                MPI_Irecv(sub_local_picture, height*stencil, MPI_INT, local_size-2, 0, image_comm, req3+2);
            }

            else{
                MPI_Isend(sub_local_picture+process_nb_pixels-2*stencil*height, height*stencil, MPI_INT, local_rank+1, 0, image_comm, req3+1); 
                MPI_Irecv(sub_local_picture+process_nb_pixels-stencil*height, height*stencil, MPI_INT, local_rank+1, 1, image_comm, req3+3); 
                MPI_Isend(sub_local_picture+height*stencil, height*stencil, MPI_INT, local_rank-1, 1, image_comm, req3); 
                MPI_Irecv(sub_local_picture, height*stencil, MPI_INT, local_rank-1, 0, image_comm, req3+2);
            }
            MPI_Waitall(4, req3, sta3);

        }
        MPI_Barrier(image_comm);

    }
    while ( threshold > 0 && !end_local) ;
    
    

    if (world_rank==world_root){
        /* IMPORT Timer start */
        gettimeofday(&t2, NULL);
        duration_blur= (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

    }

    // APPLY SOBEL FILTER 
    
    
    if ((local_rank>0)&&(local_rank<local_size-1)){
        apply_sobel_filter_mpi(sub_local_picture+(stencil-1)*height,height,process_nb_col+2);
    }
    else if (local_size==1){
        apply_sobel_filter_mpi(sub_local_picture,height,process_nb_col);
    }
    else if (local_rank==0){
        apply_sobel_filter_mpi(sub_local_picture,height,process_nb_col+1);
    }
    else{
        apply_sobel_filter_mpi(sub_local_picture+(stencil-1)*height,height,process_nb_col+1);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    

    //LOCAL ROOTS GATHER IMAGES PARTS 

    
    if (local_rank == local_root){
        req = (MPI_Request*)malloc(local_size*sizeof(MPI_Request));
        sta = (MPI_Status*)malloc(local_size*sizeof(MPI_Status));
    }
    else {
        sta=NULL;
        req=NULL;
    }

    if ((local_rank>0)&&(local_rank<local_size-1)){

        MPI_Isend(sub_local_picture+stencil*height, process_nb_pixels-2*stencil*height, MPI_INT, local_root, 0, image_comm,&req2); 
    }
    else if (local_size==1){
        MPI_Isend(sub_local_picture, width*height, MPI_INT, local_root, 0, image_comm,&req2); 
    }
    else if (local_rank==0){
        MPI_Isend(sub_local_picture, process_nb_pixels-stencil*height, MPI_INT, local_root, 0, image_comm,&req2); 
    }
    else{
        MPI_Isend(sub_local_picture+stencil*height, process_nb_pixels-stencil*height, MPI_INT, local_root, 0, image_comm,&req2); 
    }



    if (local_rank==local_root){
        if (local_size==1){
            MPI_Irecv(local_picture, process_nb_col*height, MPI_INT, 0,0, image_comm,req);
        }
        else{


            MPI_Irecv(local_picture, szbuf[0]-height*stencil, MPI_INT, 0,0, image_comm,req);

            for (k=1;k<local_size-1;k++){
                MPI_Irecv(local_picture+displs[k]+height*stencil, szbuf[k]-2*height*stencil, MPI_INT, k,0, image_comm,req+k); 
            }
            MPI_Irecv(local_picture+displs[local_size-1]+height*stencil, szbuf[local_size-1]-height*stencil, MPI_INT, local_size-1,0, image_comm,req+local_size-1);
        }
        
        MPI_Waitall(local_size, req, sta);
    }
    
    MPI_Wait(&req2, &sta2); 
    
    free(req);
    free(sta);
    
    /*
    //DEBUG
    if (local_rank==local_root){
        printf("\n First column left (after):\n");
        for (k=0;k<height;k++){
            printf("%d ",local_picture[k]);
        }
        printf("\n");

    }
    */

    //WORLD ROOT GATHER IMAGES FROM LOCAL ROOTS
    
    if (world_rank == world_root){
        buf = (int*)malloc(n_images*width*height*sizeof(int)); 
        req = (MPI_Request*)malloc(n_images*sizeof(MPI_Request));
        sta = (MPI_Status*)malloc(n_images*sizeof(MPI_Status));
    }
    else {
        buf =NULL;
        sta=NULL;
        req=NULL;
    }


    if (local_rank==local_root){
        MPI_Isend(local_picture, width*height, MPI_INT, world_root, world_rank%n_images, MPI_COMM_WORLD,&req2); 
    }
    if (world_rank==world_root){
        for (k=0;k<n_images;k++){
            MPI_Irecv(buf+k*height*width, width*height, MPI_INT, MPI_ANY_SOURCE, k, MPI_COMM_WORLD,req+k); 
        }
        MPI_Waitall(n_images, req, sta);
    }
    if (local_rank==local_root){
        MPI_Wait(&req2, &sta2); 
    }
    
    //LOCAL_ROOT PROCESS STORES FINAL IMAGE
    
    if (world_rank==world_root){
        gettimeofday(&t3, NULL);

        duration_sobel = (t3.tv_sec -t2.tv_sec)+((t3.tv_usec-t2.tv_usec)/1e6);

        //Store file from array of pixels to GIF file 
        if ( !store_pixels( output_filename, buf,n_images,width,height,g ) ) { return 1 ; }
        printf( "GIF loaded from %s \n", input_filename ) ;
        printf("Processed GIF exported in %s \n",output_filename);
        printf("Time of blur filter: %lf s  and of SOBEL filter: %lf s\n",duration_blur,duration_sobel);

    }
    
    
    MPI_Finalize();
    
    return 0 ;
}
