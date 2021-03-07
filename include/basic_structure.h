#pragma once
#include "gif_lib.h"
/* Set this macro to 1 to enable debugging information */
#define SOBELF_DEBUG 1
#define CONV(l,c,nb_c) \
    (l)*(nb_c)+(c)

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
    //pixel ** p ; /* Pixels of each image */
    int ** p; /* Pixels of each image */
    GifFileType * g ; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif ;

animated_gif* load_pixels(char * filename);

int output_modified_read_gif(char * filename, GifFileType *g);

int store_pixels(char * filename, animated_gif * image);
