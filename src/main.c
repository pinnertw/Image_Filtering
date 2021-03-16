/*
 * INF560
 *
 * Image Filtering Project
 */
#include <sys/time.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>


#include "basic_structure.h"
#include "filters.h"
/*
 * Main entry point
 */

int 
main( int argc, char ** argv )
{
    int method;
    long arg_method = 6;
    if (argc >= 4) arg_method = strtol(argv[3], NULL, 10);
    if (arg_method < INT_MIN || arg_method > INT_MAX) {
        return 1;
    }
    method = arg_method;
    if (method == 6) merge_filter(argc, argv);
    else if (method >= 3) mpi_filter(argc, argv, method - 3);
    else
    {
        char * input_filename ; 
        char * output_filename ;
        animated_gif * image ;

        /* Check command-line arguments */
        if ( argc < 3 )
        {
            fprintf( stderr, "Usage: %s input.gif output.gif (method) \n", argv[0] ) ;
            fprintf( stderr, "method : 0 classic, 1 openmp \n") ;
            return 1 ;
        }

        input_filename = argv[1] ;
        output_filename = argv[2] ;

#if time_eval
        struct timeval t1, t2;
        double duration ;
        /* IMPORT Timer start */
        gettimeofday(&t1, NULL);
#endif

        /* Load file and store the pixels in array */
        image = load_pixels( input_filename ) ;
        if ( image == NULL ) { return 1 ; }

#if time_eval
        /* IMPORT Timer stop */
        gettimeofday(&t2, NULL);

        duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

        fprintf(stderr,  "GIF loaded from file %s with %d image(s) in %lf s\n", 
                input_filename, image->n_images, duration ) ;
        printf("%s %lf ", input_filename, duration);
#endif

        if (method == 0) classic_filter(image);
        else if (method == 1) openmp_filter(image);
        else if (method == 2) cuda_filter(image);

#if time_eval
        /* EXPORT Timer start */
        gettimeofday(&t1, NULL);
#endif

        /* Store file from array of pixels to GIF file */
        if ( !store_pixels( output_filename, image ) ) { return 1 ; }

#if time_eval
        /* EXPORT Timer stop */
        gettimeofday(&t2, NULL);

        duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

        fprintf(stderr,  "Export done in %lf s in file %s\n", duration, output_filename ) ;
        printf("%lf \n", duration);
#endif
    }

    return 0 ;
}
