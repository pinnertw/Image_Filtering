make sobelf
salloc -n 3  mpirun ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpi.gif 3
#salloc -n 6 mpirun ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpi.gif 3
#export OMP_NUM_THREADS=6
salloc -n 1 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_cuda.gif 2
salloc -n 1 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 1
salloc -n 1 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_original.gif 0
rm sobelf
