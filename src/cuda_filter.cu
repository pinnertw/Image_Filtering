#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <helper_functions.h>
#include <helper_cuda.h>

#include "basic_structure.h"

__global__ void
cuda_blur_filter_kernel(int *p, int * res, int size, int threshold, int width, int height, int* end){
    int i, j, k;
    j = threadIdx.y + blockIdx.y * blockDim.y;
    k = threadIdx.x + blockIdx.x * blockDim.x;
    i = CONV(j,k,width);
    int end1 = height/10 - size;
    int end2 = height*0.9 + size;

    if (j >= 0 && j < height-1 && k >= 0 && k < width-1)
    {
        res[i] = p[i];
    }
    __syncthreads();
    /* Apply blur on top/bottom part of image (10% & 90%) */
    if (k >= size && k < width-size){
        if ((j >= size && j < end1) || (j >= end2 && j < height-size))
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
            /* Test the end condition on the variables that we have changed */
            float diff_r;
            diff_r = (res[i] - p[i]);
            if ( diff_r > threshold || -diff_r > threshold) {
                *end = 0;
            }
        }
        /* Copy the middle part of the image */
        else if (j >= end1 && j < end2){
            res[i] = p[i];
        }
    }
    __syncthreads();
    /* If the difference is large enough, we are going to reblur the image */
    if (j >= 1 && j < height - 1 && k >= 1 && k < width - 1)
    {
        p[i] = res[i] ;
    }
    __syncthreads();
}
__global__
void cuda_sobel_filter_kernel(int* p, int* res, int width, int height){
    int i, j, k;
    j = threadIdx.y + blockIdx.y * blockDim.y;
    k = threadIdx.x + blockIdx.x * blockDim.x;
    i = CONV(j,k,width);
    if (j >= 1 && j < height - 1 && k >= 1 && k < width-1)
    {
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
        if ( val_blue > 50 ) res[i] = 255 ;
        else res[i] = 0 ;
    }
    else if (j < height && k < width)
    {
        res[i] = p[i];
    }
    __syncthreads();
}

extern "C"
{
    int cuda_blur_in_while(int* p, int size, int threshold, int width, int height)
    {
        cudaSetDevice(0);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);

        int total_size = width * height * sizeof(int);

        dim3 dimBlock(
                min(32, width), 
                min(32, height)
                );
        dim3 dimGrid(
                width / dimBlock.x + 1,
                height / dimBlock.y + 1
                );
#if CUDA_DEBUG
        printf("\nSize dimBlock : %d x %d \n", dimBlock.x, dimBlock.y);
        printf("Size dimGrid : %d x %d \n", dimGrid.x, dimGrid.y);
        printf("Threads needed : %d, Threads had : %d \n", width * height, dimBlock.x*dimBlock.y*dimGrid.x * dimGrid.y);
#endif
        /* Define device variables */
        int * d_p;
        int * d_res;
        int * d_end;

        /* Allocation of memory */
        checkCudaErrors(cudaMalloc( &d_p, total_size));
        checkCudaErrors(cudaMalloc( &d_res, total_size));
        checkCudaErrors(cudaMalloc( &d_end, sizeof(int)));

        /* Copy array from CPU to device */
        checkCudaErrors(cudaMemcpy(d_p, p, total_size, cudaMemcpyHostToDevice));

        /* execute the kernel */
        int end;
        end = 1;
        cudaMemcpy(d_end, &end, sizeof(int), cudaMemcpyHostToDevice);
        cuda_blur_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, size, threshold, width, height, d_end);
        cudaMemcpy(&end, d_end, sizeof(int), cudaMemcpyDeviceToHost);

        /* return the result from device to CPU */
        checkCudaErrors(cudaMemcpy(p, d_res, total_size, cudaMemcpyDeviceToHost));
        checkCudaErrors(cudaFree(d_p));
        checkCudaErrors(cudaFree(d_res));
        checkCudaErrors(cudaFree(d_end));
        return end;
    }

    void cuda_blur_filter_per_image(int* p, int size, int threshold, int width, int height)
    {
        cudaSetDevice(0);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);

        int total_size = width * height * sizeof(int);

        dim3 dimBlock(
                min(32, width), 
                min(32, height)
                );
        dim3 dimGrid(
                width / dimBlock.x + 1,
                height / dimBlock.y + 1
                );
#if CUDA_DEBUG
        printf("\nSize dimBlock : %d x %d \n", dimBlock.x, dimBlock.y);
        printf("Size dimGrid : %d x %d \n", dimGrid.x, dimGrid.y);
        printf("Threads needed : %d, Threads had : %d \n", width * height, dimBlock.x*dimBlock.y*dimGrid.x * dimGrid.y);
#endif
        /* Define device variables */
        int * d_p;
        int * d_res;
        int * d_end;

        /* Allocation of memory */
        checkCudaErrors(cudaMalloc( &d_p, total_size));
        checkCudaErrors(cudaMalloc( &d_res, total_size));
        checkCudaErrors(cudaMalloc( &d_end, sizeof(int)));

        /* Copy array from CPU to device */
        checkCudaErrors(cudaMemcpy(d_p, p, total_size, cudaMemcpyHostToDevice));

        /* execute the kernel */
        int num_iter = 0;
        int end;
        do{
            end = 1;
            num_iter++;
            cudaMemcpy(d_end, &end, sizeof(int), cudaMemcpyHostToDevice);
            cuda_blur_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, size, threshold, width, height, d_end);
            cudaMemcpy(&end, d_end, sizeof(int), cudaMemcpyDeviceToHost);
        }while (threshold > 0 && !end);
#if CUDA_DEBUG
        printf("\nBlur filtering...Done! %d \n", num_iter);
#endif

        /* return the result from device to CPU */
        checkCudaErrors(cudaMemcpy(p, d_res, total_size, cudaMemcpyDeviceToHost));
        checkCudaErrors(cudaFree(d_p));
        checkCudaErrors(cudaFree(d_res));
        checkCudaErrors(cudaFree(d_end));
    }

    void cuda_sobel_filter_per_image(int* p, int width, int height){
        cudaSetDevice(0);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);

        int total_size = width * height * sizeof(int);
        //printf("\n %d \n", total_size);

        dim3 dimBlock(
                min(32, width), 
                min(32, height)
                );
        dim3 dimGrid(
                width / dimBlock.x + 1,
                height / dimBlock.y + 1
                );
#if CUDA_DEBUG
        printf("\nSize dimBlock : %d x %d \n", dimBlock.x, dimBlock.y);
        printf("Size dimGrid : %d x %d \n", dimGrid.x, dimGrid.y);
        printf("Threads needed : %d, Threads had : %d \n", width * height, dimBlock.x*dimBlock.y*dimGrid.x * dimGrid.y);
#endif
        /* Define device variables */
        int * d_p;
        int * d_res;

        /* Allocation of memory */
        checkCudaErrors(cudaMalloc( &d_p, total_size));
        checkCudaErrors(cudaMalloc( &d_res, total_size));

        /* Copy array from CPU to device */
        checkCudaErrors(cudaMemcpy(d_p, p, total_size, cudaMemcpyHostToDevice));

        /* execute the kernel */
        cuda_sobel_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, width, height);

        /* return the result from device to CPU */
        checkCudaErrors(cudaMemcpy(p, d_res, total_size, cudaMemcpyDeviceToHost));
        checkCudaErrors(cudaFree(d_p));
        checkCudaErrors(cudaFree(d_res));
    }

    void cuda_filter_per_image(int* p, int size, int threshold, int width, int height){
        cudaSetDevice(0);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);
        cudaEvent_t start, stop;

        int total_size = width * height * sizeof(int);
        //printf("\n %d \n", total_size);

        dim3 dimBlock(
                min(32, width), 
                min(32, height)
                );
        dim3 dimGrid(
                width / dimBlock.x + 1,
                height / dimBlock.y + 1
                );
#if CUDA_DEBUG
        printf("\nSize dimBlock : %d x %d \n", dimBlock.x, dimBlock.y);
        printf("Size dimGrid : %d x %d \n", dimGrid.x, dimGrid.y);
        printf("Threads needed : %d, Threads had : %d \n", width * height, dimBlock.x*dimBlock.y*dimGrid.x * dimGrid.y);
#endif
        /* Define device variables */
        int * d_p;
        int * d_res;
        int * d_end;

        /* Allocation of memory */
        checkCudaErrors(cudaEventCreate(&start));
        checkCudaErrors(cudaMalloc( &d_p, total_size));
        checkCudaErrors(cudaMalloc( &d_res, total_size));
        checkCudaErrors(cudaMalloc( &d_end, sizeof(int)));
        checkCudaErrors(cudaEventCreate(&stop));

        /* Copy array from CPU to device */
        checkCudaErrors(cudaMemcpy(d_p, p, total_size, cudaMemcpyHostToDevice));

        /* execute the kernel */
        int num_iter = 0;
        int end;
        do{
            end = 1;
            num_iter++;
            cudaMemcpy(d_end, &end, sizeof(int), cudaMemcpyHostToDevice);
            cuda_blur_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, size, threshold, width, height, d_end);
            cudaMemcpy(&end, d_end, sizeof(int), cudaMemcpyDeviceToHost);
        }while (threshold > 0 && !end);
#if CUDA_DEBUG
        printf("\nBlur filtering...Done! %d \n", num_iter);
#endif

        cuda_sobel_filter_kernel<<<dimGrid, dimBlock>>>(d_p, d_res, width, height);

        /* return the result from device to CPU */
        checkCudaErrors(cudaMemcpy(p, d_res, total_size, cudaMemcpyDeviceToHost));
        checkCudaErrors(cudaFree(d_p));
        checkCudaErrors(cudaFree(d_res));
        checkCudaErrors(cudaFree(d_end));
    }

    void cuda_filter( animated_gif * image){
#if SOBELF_DEBUG
        fprintf(stderr, "\nUsing CUDA functions\n");
#endif

#if time_eval
        printf("%s ", "CUDA");
        struct timeval t1, t2;
        double duration;
        /* FILTER Timer start */
        gettimeofday(&t1, NULL);
#endif

    // Apply cuda filter
        int i, width, height;
        int ** p;
        p = image->p;
        for(i=0; i<image->n_images; i++){
            width = image->width[i];
            height = image->height[i];
            //cuda_filter_per_image(p[i], 5, 20, width, height);

#if time_eval_filters
    struct timeval t3, t4;
    double duration2;
    gettimeofday(&t3, NULL);
#endif

            cuda_blur_filter_per_image(p[i], 5,20,width, height);

#if time_eval_filters
            gettimeofday(&t4, NULL);
            duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
            fprintf(stderr,  "Blur filter done in %lf s\n", duration2);
            printf("%lf ", duration2);
            gettimeofday(&t3, NULL);
#endif

            cuda_sobel_filter_per_image(p[i], width, height);

#if time_eval_filters
            gettimeofday(&t4, NULL);
            duration2 = (t4.tv_sec -t3.tv_sec)+((t4.tv_usec-t3.tv_usec)/1e6);
            fprintf(stderr,  "Sobel filter done in %lf s\n", duration2) ;
            printf("%lf ", duration2);
#endif
        }

#if time_eval
        /* FILTER Timer stop */
        gettimeofday(&t2, NULL);

        duration = (t2.tv_sec -t1.tv_sec)+((t2.tv_usec-t1.tv_usec)/1e6);

        fprintf(stderr,  "SOBEL done in %lf s\n", duration ) ;
        printf("%lf \n", duration);
#endif
    }
}
