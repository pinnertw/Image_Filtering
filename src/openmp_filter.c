#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include "basic_structure.h"
#include "filters.h"
#include "omp.h"

int openmp_blur_in_while(int* p, int* new, int size, int threshold, int width, int height)
{
    int end = 1;
    int j, k;
    int end1 = height/10 - size;
    int end2 = height*0.9 + size;
#pragma omp parallel for private (j, k)
    for(k=0; k<width-1; k++)
    {
        for (j=0; j<height-1; j++)
        {
            new[CONV(j,k,width)] = p[CONV(j,k,width)];
        }
    }
/* Apply blur on top part of image (10%) */
#pragma omp parallel for private(j, k)
    for(k=size; k<width-size; k++)
    {
        for(j=size; j<end1; j++)
        {
            int stencil_j, stencil_k ;
            int t_r = 0 ;

            for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
            {
                for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                {
                    t_r += p[CONV(j+stencil_j,k+stencil_k,width)] ;
                }
            }

            new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
        }
    }

/* Copy the middle part of the image */
#pragma omp parallel for private(j, k)
    for(k=size; k<width-size; k++)
    {
        for(j=end1; j<end2; j++)
        {
            new[CONV(j,k,width)] = p[CONV(j,k,width)] ; 
        }
    }

/* Apply blur on the bottom part of the image (10%) */
#pragma omp parallel for private(j, k)
    for(k=size; k<width-size; k++)
    {
        for(j=end2; j<height-size; j++)
        {
            int stencil_j, stencil_k ;
            int t_r = 0 ;

            for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
            {
                for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                {
                    t_r += p[CONV(j+stencil_j,k+stencil_k,width)] ;
                }
            }

            new[CONV(j,k,width)] = t_r / ( (2*size+1)*(2*size+1) ) ;
        }
    }

#pragma omp parallel for private(j, k) reduction(*: end)
    for(k=1; k<width-1; k++)
    {
        for(j=1; j<height-1; j++)
        {
            float diff_r ;

            diff_r = (new[CONV(j  ,k  ,width)] - p[CONV(j  ,k  ,width)]) ;

            if ( diff_r > threshold || -diff_r > threshold 
               ) {
                end = 0 ;
            }

            p[CONV(j  ,k  ,width)] = new[CONV(j  ,k  ,width)] ;
        }
    }

    return end;
}

void openmp_blur_filter_per_image(int* p, int size, int threshold, int width, int height)
{
    int j, k;
    int n_iter = 0 ;
    int end = 0;
    int * new;

    /* Allocate array of new pixels */
    new = (int *)malloc(width * height * sizeof( int ) ) ;

    /* Perform at least one blur iteration */
    do
    {
        n_iter++ ;
        end = openmp_blur_in_while(p, new, size, threshold, width, height);
    }
    while ( threshold > 0 && !end ) ;
    free (new);
#if SOBELF_DEBUG
    fprintf(stderr, "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif
}

void openmp_sobel_filter_per_image(int* p, int width, int height){
    int j, k;
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

            pixel_blue_no = p[CONV(j-1,k-1,width)] ;
            pixel_blue_n  = p[CONV(j-1,k  ,width)] ;
            pixel_blue_ne = p[CONV(j-1,k+1,width)] ;
            pixel_blue_so = p[CONV(j+1,k-1,width)] ;
            pixel_blue_s  = p[CONV(j+1,k  ,width)] ;
            pixel_blue_se = p[CONV(j+1,k+1,width)] ;
            pixel_blue_o  = p[CONV(j  ,k-1,width)] ;
            pixel_blue    = p[CONV(j  ,k  ,width)] ;
            pixel_blue_e  = p[CONV(j  ,k+1,width)] ;

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
            p[CONV(j  ,k  ,width)] = sobel[CONV(j  ,k  ,width)] ;
        }
    }
    free (sobel) ;
}

void openmp_blur_filter(animated_gif *image, int size, int threshold){
    int i;
    int width, height ;

    int ** p ;

    /* Get the pixels of all images */
    p = image->p ;
    
    int nb_threads;
#pragma omp parallel //just to get the number of threads
    {
        nb_threads=omp_get_num_threads();
    }

    int cut_part = image->n_images - image->n_images % nb_threads;
    //printf("\n %d %d %d \n", nb_threads, cut_part, image->n_images % nb_threads);

    /* Process all images */
#pragma omp parallel for private(i, width, height)
    for ( i = 0 ; i < cut_part ; i++ )
    {
        width = image->width[i] ;
        height = image->height[i] ;
        classic_blur_filter_per_image(p[i], size, threshold, width, height);
    }

    for ( i = cut_part ; i < image->n_images ; i++ )
    {
        width = image->width[i] ;
        height = image->height[i] ;
        openmp_blur_filter_per_image(p[i], size, threshold, width, height);
    }
}

void openmp_sobel_filter(animated_gif * image)
{
    int i, j, k;
    int width, height;
    int ** p;
    p = image->p;
    int nb_threads;
#pragma omp parallel //just to get the number of threads
    {
        nb_threads=omp_get_num_threads();
    }
    
    int cut_part = image->n_images - image->n_images % nb_threads;
#pragma omp parallel for private(i, j, k, width, height)
    for ( i = 0 ; i < cut_part ; i++ )
    {
        width = image->width[i] ;
        height = image->height[i] ;
        classic_sobel_filter_per_image(p[i], width, height);
    }
    for ( i = cut_part ; i < image->n_images ; i++ )
    {
        width = image->width[i] ;
        height = image->height[i] ;
        openmp_sobel_filter_per_image(p[i], width, height);
    }
}

void openmp_filter( animated_gif * image){
#if SOBELF_DEBUG
    fprintf(stderr, "\nUsing openmp functions\n");
#endif

#if time_eval_both
    printf("%s-%d ", "OpenMP", omp_get_max_threads());
    struct timeval t1, t2;
    double duration;
    /* FILTER Timer start */
    gettimeofday(&t1, NULL);
#endif

#if time_eval_filters
    struct timeval t3, t4;
    double duration2;
    gettimeofday(&t3, NULL);
#endif

// Apply blur filter
    openmp_blur_filter(image, 5, 20);

#if time_eval_filters
    gettimeofday(&t4, NULL);
    duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Blur filter done in %lf s\n", duration2) ;
    printf("%lf ", duration2);
    gettimeofday(&t3, NULL);
#endif

// Apply Sobel filter
    openmp_sobel_filter(image);

#if time_eval_filters
    gettimeofday(&t4, NULL);
    duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Sobel filter done in %lf s\n", duration2 ) ;
    printf("%lf ", duration2);
#endif

#if time_eval_both
    /* FILTER Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

    fprintf(stderr,  "SOBEL done in %lf s\n", duration ) ;
    printf("%lf \n", duration);
#endif
}
