# Image_Filtering

## Installation
```
make
```
Remember to set up cuda/mpi variables by running "source set_env.sh" with computers at Polytechnique or any way you install the cuda/mpi library.


## Usage
```
./sobelf input.gif output.gif method
```
with method
 - 0 : classic
 - 1 : Openmp
 - 2 : Cuda
 - 3 : MPI
 - 4 : MPI+Openmp
 - 5 : MPI+Cuda
 - 6 : merge (Default)

Time evaluation variables are set in "include/basic_structure.h", with 
 - time_eval : import/export time, total time of filters.
 - time_eval_filters : blur filter time/sobel filter time (attention, with cuda method it is going to generate more than 1 output since we didn't seperate two filters).
 - time_eval_filters : only total time of filters.

'images/test_single_image/create_gif.py': randomly generated animated gifs with **one** picture of fixed width and different heights

Case1 and case2 in mpi_filter.c correspond to Step 1 and step 2 in the report.

in openmp_filter.c:
Step1: we distribute the first k * nbThreads images of an animated gif over OpenMP threads, where k= nbImages// nbThreads 
Step2: we process sequentially the remaining nbImages % nbThreads images, but we use OpenMP threads to parallelize the processing of the pixels within each image.

## Test
Before testing with following scripts, set up variables time_eval and time_eval_filters to 1 in the file "include/basic_structure.h"
### Heatmap
We can use the following script to do the heatmap test between openmp and cuda method.
```
./test_script/single_machine_heatmap.sh > output
```
### Single machine test
With -N 1 variable, we can test different method on one machine.
#### Single image
```
./test_script/single_machine_test.sh height_min step > output
```
then it will test every method on image varying from height_min, height_min+step... to height_min+10\*step.
#### Multiple image
```
./test_script/single_machine_multiple_images.sh > output
```
### Multiple machine test
With -N 2/3 variable, we test different MPI methods on several machine.
```
./test_script/multiple_machine_test.sh height_min step> output
```
the height_min and step are the same as single machine test. We test on 11 gif.

each line of multiple_machine_test is in the form: name load_time method blur sobel (blur+sobel) export_time
### Analyse
In result/, you can find out some jupyter notebooks and some tests output for each test.
