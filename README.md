# Image_Filtering

Install :
make (Remember to set up cuda/mpi variables)

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


chaque ligne de multiple_machine_test: name load_time method blur sobel export_time
