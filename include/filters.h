void classic_blur_filter_per_image(int * p, int size, int threshold, int width, int height);
void classic_blur_filter(animated_gif * image, int size, int threshold);
void classic_sobel_filter_per_image(int * p, int width, int height);
void classic_sobel_filter(animated_gif * image);
void classic_filter(animated_gif * image);
void openmp_blur_filter(animated_gif *image, int size, int threshold);
void openmp_blur_filter_per_image(int * p, int size, int threshold, int width, int height);
void openmp_sobel_filter_per_image(int* p, int width, int height);
void openmp_sobel_filter(animated_gif *image);
void openmp_filter(animated_gif *image);
