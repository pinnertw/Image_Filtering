# Image_Filtering

Install :
make (Remember to set up cuda/mpi variables by running "source set_env.sh")

For testing :
./multiple_machine_test > output
./single_machine_test > output

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

MPI_filter: 

- Step 1 (repeat while nb_process<nb remaining images): process the first nb_process images by assigning 1 process per image; update nb remaining images <- nb remaining images-nb_proocess
- Step 2 (directly step 2 if nb_process>=nb_images): process the remaining images with all processes (not necessarily 1 process per image this time) 
