#include "basic_structure.h"

extern void openmp_one_image_gray_filter_one_thread(int i, int width, int height, pixel** p);
extern void openmp_one_image_blur_filter_one_thread(int i, int width, int height, int size, int threshold, pixel** p);
extern void openmp_one_image_sobelf_filter_one_thread(int i, int width, int height, pixel** p);

