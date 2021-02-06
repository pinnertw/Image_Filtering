#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include "basic_structure.h"
#include "openmp.h"

void openmp_one_image_gray_filter_one_thread(int i, int width, int height, pixel** p){
    int j;
    // Apply gray filter
    for (j = 0; j < width*height; j++){
        int moy;
        moy = (p[i][j].r + p[i][j].g + p[i][j].b)/3;
        if (moy < 0) moy = 0;
        if (moy > 255) moy = 255;
        p[i][j].r = moy;
        p[i][j].g = moy;
        p[i][j].b = moy;
    }
}

#define CONV(l, c, nb_c) \
    (l)*(nb_c)+(c)

void openmp_one_image_blur_filter_one_thread(int i, int width, int height, int size, int threshold, pixel** p){
    int j, k;
    int end=0;
    int n_iter=0;
    pixel * new;
    /* Allocate array of new pixels */
    new = (pixel *)malloc(width * height * sizeof(pixel));
        /* Perform at least one blur iteration */
        do
        {
            end = 1 ;
            n_iter++ ;


            for(j=0; j<height-1; j++)
            {
                for(k=0; k<width-1; k++)
                {
                    new[CONV(j,k,width)].r = p[i][CONV(j,k,width)].r ;
                    new[CONV(j,k,width)].g = p[i][CONV(j,k,width)].g ;
                    new[CONV(j,k,width)].b = p[i][CONV(j,k,width)].b ;
                }
            }

            /* Apply blur on top part of image (10%) */
            for(j=size; j<height/10-size; j++)
            {
                for(k=size; k<width-size; k++)
                {
                    int stencil_j, stencil_k ;
                    int t_r = 0 ;
                    int t_g = 0 ;
                    int t_b = 0 ;

                    for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                    {
                        for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                        {
                            t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)].r ;
                            t_g += p[i][CONV(j+stencil_j,k+stencil_k,width)].g ;
                            t_b += p[i][CONV(j+stencil_j,k+stencil_k,width)].b ;
                        }
                    }

                    new[CONV(j,k,width)].r = t_r / ( (2*size+1)*(2*size+1) ) ;
                    new[CONV(j,k,width)].g = t_g / ( (2*size+1)*(2*size+1) ) ;
                    new[CONV(j,k,width)].b = t_b / ( (2*size+1)*(2*size+1) ) ;
                }
            }

            /* Copy the middle part of the image */
            for(j=height/10-size; j<height*0.9+size; j++)
            {
                for(k=size; k<width-size; k++)
                {
                    new[CONV(j,k,width)].r = p[i][CONV(j,k,width)].r ; 
                    new[CONV(j,k,width)].g = p[i][CONV(j,k,width)].g ; 
                    new[CONV(j,k,width)].b = p[i][CONV(j,k,width)].b ; 
                }
            }

            /* Apply blur on the bottom part of the image (10%) */
            for(j=height*0.9+size; j<height-size; j++)
            {
                for(k=size; k<width-size; k++)
                {
                    int stencil_j, stencil_k ;
                    int t_r = 0 ;
                    int t_g = 0 ;
                    int t_b = 0 ;

                    for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                    {
                        for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                        {
                            t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)].r ;
                            t_g += p[i][CONV(j+stencil_j,k+stencil_k,width)].g ;
                            t_b += p[i][CONV(j+stencil_j,k+stencil_k,width)].b ;
                        }
                    }

                    new[CONV(j,k,width)].r = t_r / ( (2*size+1)*(2*size+1) ) ;
                    new[CONV(j,k,width)].g = t_g / ( (2*size+1)*(2*size+1) ) ;
                    new[CONV(j,k,width)].b = t_b / ( (2*size+1)*(2*size+1) ) ;
                }
            }

            for(j=1; j<height-1; j++)
            {
                for(k=1; k<width-1; k++)
                {

                    float diff_r ;
                    float diff_g ;
                    float diff_b ;

                    diff_r = (new[CONV(j  ,k  ,width)].r - p[i][CONV(j  ,k  ,width)].r) ;
                    diff_g = (new[CONV(j  ,k  ,width)].g - p[i][CONV(j  ,k  ,width)].g) ;
                    diff_b = (new[CONV(j  ,k  ,width)].b - p[i][CONV(j  ,k  ,width)].b) ;

                    if ( diff_r > threshold || -diff_r > threshold 
                            ||
                             diff_g > threshold || -diff_g > threshold
                             ||
                              diff_b > threshold || -diff_b > threshold
                       ) {
                        end = 0 ;
                    }

                    p[i][CONV(j  ,k  ,width)].r = new[CONV(j  ,k  ,width)].r ;
                    p[i][CONV(j  ,k  ,width)].g = new[CONV(j  ,k  ,width)].g ;
                    p[i][CONV(j  ,k  ,width)].b = new[CONV(j  ,k  ,width)].b ;
                }
            }
        }
        while ( threshold > 0 && !end ) ;

#if SOBELF_DEBUG
	printf( "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif
        free (new) ;
}

void openmp_one_image_sobelf_filter_one_thread(int i, int width, int height, pixel** p)
{
    int j, k ;
    pixel * sobel ;

    sobel = (pixel *)malloc(width * height * sizeof( pixel ) ) ;

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

            pixel_blue_no = p[i][CONV(j-1,k-1,width)].b ;
            pixel_blue_n  = p[i][CONV(j-1,k  ,width)].b ;
            pixel_blue_ne = p[i][CONV(j-1,k+1,width)].b ;
            pixel_blue_so = p[i][CONV(j+1,k-1,width)].b ;
            pixel_blue_s  = p[i][CONV(j+1,k  ,width)].b ;
            pixel_blue_se = p[i][CONV(j+1,k+1,width)].b ;
            pixel_blue_o  = p[i][CONV(j  ,k-1,width)].b ;
            pixel_blue    = p[i][CONV(j  ,k  ,width)].b ;
            pixel_blue_e  = p[i][CONV(j  ,k+1,width)].b ;

            deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

            deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

            val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;


            if ( val_blue > 50 ) 
            {
                sobel[CONV(j  ,k  ,width)].r = 255 ;
                sobel[CONV(j  ,k  ,width)].g = 255 ;
                sobel[CONV(j  ,k  ,width)].b = 255 ;
            } else
            {
                sobel[CONV(j  ,k  ,width)].r = 0 ;
                sobel[CONV(j  ,k  ,width)].g = 0 ;
                sobel[CONV(j  ,k  ,width)].b = 0 ;
            }
        }
    }
        //printf("\n Fin de sobel processing \n");

    for(j=1; j<height-1; j++)
    {
        for(k=1; k<width-1; k++)
        {
            p[i][CONV(j  ,k  ,width)].r = sobel[CONV(j  ,k  ,width)].r ;
            p[i][CONV(j  ,k  ,width)].g = sobel[CONV(j  ,k  ,width)].g ;
            p[i][CONV(j  ,k  ,width)].b = sobel[CONV(j  ,k  ,width)].b ;
        }
    }

    free (sobel) ;
}
