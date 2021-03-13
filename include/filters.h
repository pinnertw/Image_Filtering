// Classical filters
void classic_blur_filter_per_image(int * p, int size, int threshold, int width, int height);
void classic_blur_filter(animated_gif * image, int size, int threshold);
void classic_sobel_filter_per_image(int * p, int width, int height);
void classic_sobel_filter(animated_gif * image);
void classic_filter(animated_gif * image);

// Openmp filters
void openmp_blur_filter(animated_gif *image, int size, int threshold);
void openmp_blur_filter_per_image(int * p, int size, int threshold, int width, int height);
void openmp_sobel_filter_per_image(int* p, int width, int height);
void openmp_sobel_filter(animated_gif *image);
void openmp_filter(animated_gif *image);

// Cuda filters
#ifdef __cplusplus
extern "C"
{
#endif

    void cuda_filter_per_image(int* p, int size, int threshold, int width, int height);
    void cuda_filter(animated_gif *image);

#ifdef __cplusplus
}
#endif

// MPI filters
int mpi_filter_rank_0(animated_gif * image);
int mpi_filter_other_rank();
int mpi_filter(int argc, char ** argv, int method);
