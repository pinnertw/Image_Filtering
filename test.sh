make sobelf_openmp
export OMP_NUM_THREADS=3
echo $OMP_NUM_THREADS
./sobelf_openmp images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif
#export OMP_NUM_THREADS=3
#echo $OMP_NUM_THREADS
#./sobelf_openmp images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif
export OMP_NUM_THREADS=1
echo $OMP_NUM_THREADS
./sobelf_openmp images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output.gif
rm sobelf_openmp
#make sobelf
#./sobelf images/original/Campusplan-Mobilitaetsbeschraenkte.gif images/processed/1_output_original.gif
#rm sobelf
