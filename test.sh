make sobelf
#salloc -n 6  mpirun ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpi.gif 3
#salloc -n 1 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_cuda.gif 2
export OMP_NUM_THREADS=6
#mpirun -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpiomp.gif 4
#./sobelf images/original/campusplan-mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 1
#mpirun -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpiomp.gif 4
export OMP_NUM_THREADS=2
#mpirun -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpiomp.gif 4
#mpirun -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpicuda.gif 5
#mpirun -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpi.gif 3
mpirun -n 3 ./sobelf images/original/1.gif images/processed/1_output_mpi.gif 3
mpirun -n 3 ./sobelf images/original/1.gif images/processed/1_output_mpiomp.gif 4
mpirun -n 3 ./sobelf images/original/1.gif images/processed/1_output_mpicuda.gif 5
#salloc -n 3 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_mpi.gif 3
#salloc -n 1 ./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_original.gif 0
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 1
rm sobelf
