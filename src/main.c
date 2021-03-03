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

#define CONV(l,c,nb_c) \
    (l)*(nb_c)+(c)

void
apply_blur_filter( animated_gif * image, int size, int threshold )
{
    int i, j, k ;
    int width, height ;
    int end = 0 ;
    int n_iter = 0 ;

    int ** p ;
    int * new ;

    /* Get the pixels of all images */
    p = image->p ;


    /* Process all images */
    for ( i = 0 ; i < image->n_images ; i++ )
    {
        n_iter = 0 ;
        width = image->width[i] ;
        height = image->height[i] ;

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
			new[CONV(j,k,width)]  = p[i][CONV(j,k,width)]  ;
		}
	}

            /* Apply blur on top part of image (10%) */
            for(j=size; j<height/10-size; j++)
            {
                for(k=size; k<width-size; k++)
                {
                    int stencil_j, stencil_k ;
                    int t_r = 0 ;

                    for ( stencil_j = -size ; stencil_j <= size ; stencil_j++ )
                    {
                        for ( stencil_k = -size ; stencil_k <= size ; stencil_k++ )
                        {
                            t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)]  ;
                        }
                    }

                    new[CONV(j,k,width)]  = t_r / ( (2*size+1)*(2*size+1) ) ;
                }
            }

            /* Copy the middle part of the image */
            for(j=height/10-size; j<height*0.9+size; j++)
            {
                for(k=size; k<width-size; k++)
                {
                    new[CONV(j,k,width)]  = p[i][CONV(j,k,width)]  ; 
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
                            t_r += p[i][CONV(j+stencil_j,k+stencil_k,width)]  ;
                        }
                    }

                    new[CONV(j,k,width)]  = t_r / ( (2*size+1)*(2*size+1) ) ;
                }
            }

            for(j=1; j<height-1; j++)
            {
                for(k=1; k<width-1; k++)
                {

                    float diff_r ;

                    diff_r = (new[CONV(j  ,k  ,width)]  - p[i][CONV(j  ,k  ,width)] ) ;

                    if ( diff_r > threshold || -diff_r > threshold 
                       ) {
                        end = 0 ;
                    }

                    p[i][CONV(j  ,k  ,width)]  = new[CONV(j  ,k  ,width)]  ;
                }
            }

        }
        while ( threshold > 0 && !end ) ;

#if SOBELF_DEBUG
	printf( "BLUR: number of iterations for image %d\n", n_iter ) ;
#endif

        free (new) ;
    }

}

void
apply_sobel_filter( animated_gif * image )
{
    int i, j, k ;
    int width, height ;

    int ** p ;

    p = image->p ;

    for ( i = 0 ; i < image->n_images ; i++ )
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

                pixel_blue_no = p[i][CONV(j-1,k-1,width)]  ;
                pixel_blue_n  = p[i][CONV(j-1,k  ,width)]  ;
                pixel_blue_ne = p[i][CONV(j-1,k+1,width)]  ;
                pixel_blue_so = p[i][CONV(j+1,k-1,width)]  ;
                pixel_blue_s  = p[i][CONV(j+1,k  ,width)]  ;
                pixel_blue_se = p[i][CONV(j+1,k+1,width)]  ;
                pixel_blue_o  = p[i][CONV(j  ,k-1,width)]  ;
                pixel_blue    = p[i][CONV(j  ,k  ,width)]  ;
                pixel_blue_e  = p[i][CONV(j  ,k+1,width)]  ;

                deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

                deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

                val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;


                if ( val_blue > 50 ) 
                {
                    sobel[CONV(j  ,k  ,width)]  = 255 ;
                } else
                {
                    sobel[CONV(j  ,k  ,width)]  = 0 ;
                }
            }
        }

        for(j=1; j<height-1; j++)
        {
            for(k=1; k<width-1; k++)
            {
                p[i][CONV(j  ,k  ,width)]  = sobel[CONV(j  ,k  ,width)]  ;
            }
        }

        free (sobel) ;
    }

}

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
    fprintf(stderr, "\nTest classical functions \n");
    printf("%s ", "classic");
    gettimeofday(&t1, NULL);

    /* Apply blur filter with convergence value */
    gettimeofday(&t3, NULL);

    apply_blur_filter( image, 5, 20 ) ;

    gettimeofday(&t4, NULL);
    duration = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Blur filter done in %lf s\n", duration ) ;
    printf("%lf ", duration);

    /* Apply sobel filter on pixels */
    gettimeofday(&t3, NULL);

    apply_sobel_filter( image ) ;

    gettimeofday(&t4, NULL);
    duration = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
    fprintf(stderr,  "Sobel filter done in %lf s\n", duration ) ;
    printf("%lf ", duration);

    /* FILTER Timer stop */
    fprintf(stderr, "Test classical functions done \n\n");
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
