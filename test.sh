#make sobelf
#export OMP_NUM_THREADS=6
#echo $OMP_NUM_THREADS
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif 1
#export OMP_NUM_THREADS=1
#echo $OMP_NUM_THREADS
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif 1
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_original.gif 0
make sobelf_gpu
./sobelf_gpu images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_gpu.gif 2
./sobelf_gpu images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_openmp.gif 1
rm sobelf_gpu
#rm sobelf
