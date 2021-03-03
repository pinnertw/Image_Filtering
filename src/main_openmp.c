/*
 * INF560
 *
 * Image Filtering Project
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "gif_lib.h"
#include "basic_structure.h"
#include "omp.h"

#define CONV(l,c,nb_c) \
    (l)*(nb_c)+(c)

/*
 * Main entry point
 */
int 
main( int argc, char ** argv )
{
    char * input_filename ; 
    char * output_filename ;
    animated_gif * image ;
    struct timeval t1, t2, t3, t4;
    double duration ;

    /* Check command-line arguments */
    if ( argc < 3 )
    {
        fprintf( stderr, "Usage: %s input.gif output.gif \n", argv[0] ) ;
        return 1 ;
    }

    input_filename = argv[1] ;
    output_filename = argv[2] ;

    /* IMPORT Timer start */
    gettimeofday(&t1, NULL);

    /* Load file and store the pixels in array */
    image = load_pixels( input_filename ) ;
    if ( image == NULL ) { return 1 ; }

    /* IMPORT Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

    fprintf(stderr,  "GIF loaded from file %s with %d image(s) in %lf s\n", 
            input_filename, image->n_images, duration ) ;
    printf("%s ", input_filename);

    /* FILTER Timer start */
    gettimeofday(&t1, NULL);
    fprintf(stderr, "\nTest openmp functions\n");
    printf("%s ", "OpenMP");

// Apply blur filter
    gettimeofday(&t3, NULL);

    int size = 5;
    int threshold = 20;

    int i, j, k ;
    int width, height ;
    int end = 0 ;
    int n_iter = 0 ;

    int ** p ;
    int * new ;

    /* Get the pixels of all images */
    p = image->p ;

    int nb_threads = omp_get_max_threads();
    int cut_part = image->n_images - image->n_images % nb_threads;
//printf("\n %d %d %d \n", nb_threads, cut_part, image->n_images % nb_threads);

    /* Process all images */
#pragma omp parallel
    {
#pragma omp for private(i, j, new, width, height, n_iter, k)
        for ( i = 0 ; i < cut_part ; i++ )
        {
            n_iter = 0 ;
            width = image->width[i] ;
            height = image->height[i] ;
            end = height/10-size;

            /* Allocate array of new pixels */
            new = (int *)malloc(width * height * sizeof( int ) ) ;

            /* Perform at least one blur iteration */
            do
            {
                end = 1 ;
                n_iter++ ;


                for(j=0; j<height-1; j++)
                {
                    for(k=0; k<width-1; k++)
                    {
                        new[CONV(j,k,width)] = p[i][CONV(j,k,width)];
                    }
                }

                /* Apply blur on top part of image (10%) */
                for(j=size; j<end; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        int stencil_j, stencil_k ;
                        int t_r = 0 ;

                        for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                        {
                            for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                            {
                                t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)] ;
                            }
                        }
                        new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
                    }
                }

                /* Copy the middle part of the image */
                for(j=height/10-size; j<height*0.9+size; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        new[CONV(j,k,width)] = p[i][CONV(j,k,width)] ; 
                    }
                }

                /* Apply blur on the bottom part of the image (10%) */
                for(j=height*0.9+size; j<height-size; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        int stencil_j, stencil_k ;
                        int t_r = 0 ;

                        for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                        {
                            for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                            {
                                t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)] ;
                            }
                        }

                        new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
                    }
                }

                for(j=1; j<height-1; j++)
                {
                    for(k=1; k<width-1; k++)
                    {

                        float diff_r ;

                        diff_r = (new[CONV(j  ,k  ,width)] - p[i][CONV(j  ,k  ,width)]) ;

                        if ( diff_r > threshold || -diff_r > threshold 
                           ) {
                            end = 0 ;
                        }

                        p[i][CONV(j  ,k  ,width)] = new[CONV(j  ,k  ,width)];
                    }
                }
            }
            while ( threshold > 0 && !end ) ;
            free (new) ;
#if SOBELF_DEBUG
        printf( "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif
        }
    }
    for ( i = cut_part ; i < image->n_images ; i++ )
    {
        n_iter = 0 ;
        width = image->width[i] ;
        height = image->height[i] ;
        int end1 = height/10 - size;
        int end2 = height*0.9 + size;

        /* Allocate array of new pixels */
        new = (int *)malloc(width * height * sizeof( int ) ) ;

        /* Perform at least one blur iteration */
        do
        {
            end = 1 ;
            n_iter++ ;
#pragma omp parallel default(none) shared(new, p, i, height, width) private(j, k)
            {
#pragma omp for private(j, k)// collapse(2) schedule(static, 10)
                for(j=0; j<height-1; j++)
                {
                    for(k=0; k<width-1; k++)
                    {
                        new[CONV(j,k,width)] = p[i][CONV(j,k,width)] ;

                    }
                }
            }

            /* Apply blur on top part of image (10%) */
#pragma omp parallel default(none) shared(new, p, width, size, end1, i) private(j, k)
            {
#pragma omp for private(j, k)// collapse(2) schedule(static, 10)
                for(j=size; j<end1; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        int stencil_j, stencil_k ;
                        int t_r = 0 ;

                        for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                        {
                            for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                            {
                                t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)] ;
                            }
                        }

                        new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
                    }
                }
            }

            /* Copy the middle part of the image */
#pragma omp parallel default(none) shared(i, p, new, width, end1, end2, size) private(j, k)
            {
#pragma omp for private(j, k)// collapse(2) schedule(static, 10)
                for(j=end1; j<end2; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        new[CONV(j,k,width)] = p[i][CONV(j,k,width)] ; 
                    }
                }
            }

            /* Apply blur on the bottom part of the image (10%) */
#pragma omp parallel default(none) shared(i, end2, height, size, width, p, new) private(j, k)
            {
#pragma omp for private(j, k)// collapse(2) schedule(static, 10)
                for(j=end2; j<height-size; j++)
                {
                    for(k=size; k<width-size; k++)
                    {
                        int stencil_j, stencil_k ;
                        int t_r = 0 ;

                        for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                        {
                            for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                            {
                                t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)] ;
                            }
                        }

                        new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
                    }
                }
            }

#pragma omp parallel default(none) shared(i, width, p, new, height, threshold, end)
            {
#pragma omp for private(j, k) reduction(*: end)
                for(j=1; j<height-1; j++)
                {
                    for(k=1; k<width-1; k++)
                    {
                        float diff_r ;

                        diff_r = (new[CONV(j  ,k  ,width)] - p[i][CONV(j  ,k  ,width)]) ;

                        if ( diff_r > threshold || -diff_r > threshold 
                           ) {
                            end = 0 ;
                        }

                        p[i][CONV(j  ,k  ,width)] = new[CONV(j  ,k  ,width)] ;
                    }
                }
            }
        }
        while ( threshold > 0 && !end ) ;
        free (new);
#if SOBELF_DEBUG
        printf( "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif
    }

    gettimeofday(&t4, NULL);
    duration = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Blur filter done in %lf s\n", duration ) ;
    printf("%lf ", duration);

// Apply Sobel filter
    gettimeofday(&t3, NULL);
    {
#pragma omp parallel for private(i, j, k, width, height)
        for ( i = 0 ; i < cut_part ; i++ )
        {
            width = image->width[i] ;
            height = image->height[i] ;

            int * sobel ;

            sobel = (int *)malloc(width * height * sizeof( int ) ) ;

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

                    pixel_blue_no = p[i][CONV(j-1,k-1,width)] ;
                    pixel_blue_n  = p[i][CONV(j-1,k  ,width)] ;
                    pixel_blue_ne = p[i][CONV(j-1,k+1,width)] ;
                    pixel_blue_so = p[i][CONV(j+1,k-1,width)] ;
                    pixel_blue_s  = p[i][CONV(j+1,k  ,width)] ;
                    pixel_blue_se = p[i][CONV(j+1,k+1,width)] ;
                    pixel_blue_o  = p[i][CONV(j  ,k-1,width)] ;
                    pixel_blue    = p[i][CONV(j  ,k  ,width)] ;
                    pixel_blue_e  = p[i][CONV(j  ,k+1,width)] ;

                    deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

                    deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

                    val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;


                    if ( val_blue > 50 ) 
                    {
                        sobel[CONV(j  ,k  ,width)] = 255 ;
                    } else
                    {
                        sobel[CONV(j  ,k  ,width)] = 0 ;
                    }
                }
            }

            for(j=1; j<height-1; j++)
            {
                for(k=1; k<width-1; k++)
                {
                    p[i][CONV(j  ,k  ,width)] = sobel[CONV(j  ,k  ,width)] ;
                }
            }

            free (sobel) ;
        }
        for ( i = cut_part ; i < image->n_images ; i++ )
        {
            width = image->width[i] ;
            height = image->height[i] ;

            int * sobel ;

            sobel = (int *)malloc(width * height * sizeof( int ) ) ;

#pragma omp parallel for private(j, k)
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

                    pixel_blue_no = p[i][CONV(j-1,k-1,width)] ;
                    pixel_blue_n  = p[i][CONV(j-1,k  ,width)] ;
                    pixel_blue_ne = p[i][CONV(j-1,k+1,width)] ;
                    pixel_blue_so = p[i][CONV(j+1,k-1,width)] ;
                    pixel_blue_s  = p[i][CONV(j+1,k  ,width)] ;
                    pixel_blue_se = p[i][CONV(j+1,k+1,width)] ;
                    pixel_blue_o  = p[i][CONV(j  ,k-1,width)] ;
                    pixel_blue    = p[i][CONV(j  ,k  ,width)] ;
                    pixel_blue_e  = p[i][CONV(j  ,k+1,width)] ;

                    deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

                    deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

                    val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;


                    if ( val_blue > 50 ) 
                    {
                        sobel[CONV(j  ,k  ,width)] = 255 ;
                    } else
                    {
                        sobel[CONV(j  ,k  ,width)] = 0 ;
                    }
                }
            }
#pragma omp parallel for private(j, k)
            for(j=1; j<height-1; j++)
            {
                for(k=1; k<width-1; k++)
                {
                    p[i][CONV(j  ,k  ,width)] = sobel[CONV(j  ,k  ,width)] ;
                }
            }

            free (sobel) ;
        }
    }
    gettimeofday(&t4, NULL);
    duration = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Sobel filter done in %lf s\n", duration ) ;
    printf("%lf ", duration);

    fprintf(stderr, "Test openmp functions done\n\n");
    /* FILTER Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

    fprintf(stderr,  "SOBEL done in %lf s\n", duration ) ;
    printf("%lf ", duration);

    /* EXPORT Timer start */
    gettimeofday(&t1, NULL);

    /* Store file from array of pixels to GIF file */
    if ( !store_pixels( output_filename, image ) ) { return 1 ; }

    /* EXPORT Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

    fprintf(stderr,  "Export done in %lf s in file %s\n", duration, output_filename ) ;
    printf("%lf \n", duration);

    return 0 ;
}
