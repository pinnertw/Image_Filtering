#include <cuda.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include <cuda_runtime.h>
#include <helper_functions.h>
#include <helper_cuda.h>

#include "basic_structure.h"

__global__ void
cuda_blur_filter_kernel(int *p, int * res, int size, int threshold, int width, int height, int* end){
    int total_size = width * height;
    int index;
    int nb_threads;
    index = blockIdx.x * blockDim.x + threadIdx.x;
    nb_threads = blockDim.x * gridDim.x;
    int i, j, k;
    int end1 = height/10 - size;
    int end2 = height*0.9 + size;

    for (i = index; i < total_size; i+= nb_threads)
    {
        j = i / width;
        k = i % width;
        if (j >= 0 && j < height-1 && k >= 0 && k < width-1)
        {
        res[i] = p[i];
        }
    }
    __syncthreads();
    for (i = index; i < total_size; i+= nb_threads)
    {
        j = i / width;
        k = i % width;
        if ((j >= size && j < end1 && k >= size && k < width-size)||
         (j >= end2 && j < height-size && k >= size && k <= width-size))
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
            res[i] = t_r / ( (2*size+1)*(2*size+1) ) ;
        }
        else{
            res[i] = p[i];
        }
    }
    __syncthreads();
    for(i=index; i < total_size; i+= nb_threads)
    {
        j = i / width;
        k = i % width;
        if (j >= 1 && j < height - 1 && k >= 1 && k < width - 1)
        {
            float diff_r;
            diff_r = (res[i] - p[i]) ;
            if ( diff_r > threshold || -diff_r > threshold) {
                *end = 0;
            }
            p[i] = res[i] ;
        }
    }
    __syncthreads();
}
__global__
void cuda_sobel_filter_kernel(int* p, int* res, int width, int height){
    int i, j, k;
    int total_size = width * height;
    int index;
    int nb_threads;
    index = blockIdx.x * blockDim.x + threadIdx.x;
    nb_threads = blockDim.x * gridDim.x;
    for (i = index; i < total_size; i+= nb_threads)
    {
        j = i % height;
        k = i % width;
        if (j >= 1 && j < height - 1 && k >= 1 && k < width-1){
            int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
            int pixel_blue_so, pixel_blue_s, pixel_blue_se;
            int pixel_blue_o , pixel_blue_e ;

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
            pixel_blue_e  = p[CONV(j  ,k+1,width)] ;

            deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2*pixel_blue_o + 2*pixel_blue_e - pixel_blue_so + pixel_blue_se;             

            deltaY_blue = pixel_blue_se + 2*pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2*pixel_blue_n - pixel_blue_no;

            val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue)/4;

            if ( val_blue > 50 ) 
            {
                res[i] = 255 ;
            } else
            {
                res[i] = 0 ;
            }
        }
        else{
            res[i] = p[i];
        }
    }
    __syncthreads();
}

extern "C"
{
    void cuda_filter_per_image(int* p, int size, int threshold, int width, int height){
        cudaSetDevice(0);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);

        int total_size = width * height * sizeof(int);
        //printf("\n %d \n", total_size);

        dim3 dimBlock(deviceProp.maxThreadsPerBlock);//, deviceProp.maxThreadsDim[1]);
        dim3 dimGrid(total_size/deviceProp.maxThreadsPerBlock + 1);
        /* Define device variables */
        int * d_p;
        int * d_res;
        int * d_end;

        /* Allocation of memory */
        cudaMalloc( &d_p, total_size);
        cudaMalloc( &d_res, total_size);
        cudaMalloc( &d_end, sizeof(int));

        /* Copy array from CPU to device */
        cudaMemcpy(d_p, p, total_size, cudaMemcpyHostToDevice);

        /* execute the kernel */
        int num_iter = 0;
        int end;
        do{
            end = 1;
            num_iter++;
            printf("Blur filtering... %d \n", end);
            cudaMemcpy(d_end, &end, sizeof(int), cudaMemcpyHostToDevice);
            cuda_blur_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, size, threshold, width, height, d_end);
            cudaMemcpy(&end, d_end, sizeof(int), cudaMemcpyDeviceToHost);
            printf("Blur filtering...Done! %d \n", end);
        }while (threshold > 0 && !end);

        //cuda_sobel_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, width, height);

        /* return the result from device to CPU */
        cudaMemcpy(p, d_res, total_size, cudaMemcpyDeviceToHost);
        cudaFree(d_p);
        cudaFree(d_res);
        cudaFree(d_end);
    }

    void cuda_filter( animated_gif * image){
        struct timeval t1, t2;
        double duration;

        /* FILTER Timer start */
        gettimeofday(&t1, NULL);
        fprintf(stderr, "\nUsing cuda functions\n");
        printf("%s ", "CUDA");

    // Apply cuda filter
        int i, width, height;
        int ** p;
        p = image->p;
        for(i=0; i<image->n_images; i++){
            width = image->width[i];
            height = image->height[i];
            cuda_filter_per_image(p[i], 5, 20, width, height);
        }

        /* FILTER Timer stop */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);
        fprintf(stderr,  "SOBEL done in %lf s\n", duration ) ;
        printf("%lf ", duration);
    }
}
