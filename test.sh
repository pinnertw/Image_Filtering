#make sobelf
#export OMP_NUM_THREADS=6
#echo $OMP_NUM_THREADS
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif 1
#export OMP_NUM_THREADS=1
#echo $OMP_NUM_THREADS
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif 1
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_original.gif 0
#make sobelf_gpu
#./sobelf_gpu images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_gpu.gif 2
#./sobelf_gpu images/original/1.gif images/processed/1_output_gpu.gif 2
#./sobelf_gpu images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 1
#rm sobelf_gpu
#rm sobelf
make sobelf_mpi
#salloc -n 3  mpirun ./sobelf_mpi images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 3
#mpirun -n 3 ./sobelf_mpi images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 3
mpirun -n 3 ./sobelf_mpi images/original/1.gif images/processed/1_output_mpi.gif 3
rm sobelf_mpi

