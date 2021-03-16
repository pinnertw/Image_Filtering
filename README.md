# Image_Filtering

Install :
make (Remember to set up cuda/mpi variables by running "source set_env.sh")

For testing :
./multiple_machine_test > output
./single_machine_test > output
or ./single_machine_test.sh 200 100 > output (first parameter: height_min ; second parameter: step)
or ./single_machine_multiple_images.sh > output

For the usage of the program :
./sobelf input.gif output.gif method
with method
0 : classic
1 : Openmp
2 : Cuda
3 : MPI
4 : MPI+Openmp
5 : MPI+Cuda


each line of multiple_machine_test is in the form: name load_time method blur sobel (blur+sobel) export_time

images/test: randomly generated animated gifs with **one** picture of fixed width and different heights

Case1 and case2 in mpi_filter.c correspond to Step 1 and step 2 in the report.

in openmp_filter.c:
Step1: we distribute the first k * nbThreads images of an animated gif over OpenMP threads, where k= nbImages// nbThreads 
Step 2: we process sequentially the remaining nbImages % nbThreads images, but we use OpenMP threads to parallelize the processing of the pixels within each image.
