#/bin/sh

# $1: Numero de pruebas
# $2: Nombre del archivo
# $3: Numero de filas
# $4: Numero de columnas
# $5: Numero de filas/worker

# Compilacion
gcc MatrizPrimo.c -o secuencial
mpicc PrimoParalelo.v3.c -o Paralelo
echo "[->] Compilacion Exitosa"

# Version secuencial
echo "[->] Calculo Secuencial"
echo "[->] Calculo Secuencial" >> $2.txt
for ((i = 1; i < ($1+1); i++)); do
	echo "[->] Test $i:"
	echo "[->] Test $i:" >> $2.txt
	./secuencial $3 $4 >> $2.txt
done

# Version paralela
echo "[->] Calculo Paralelo"
echo "[->] Calculo Paralelo" >> $2.txt
for ((i = 1; i < ($1+1); i++)); do
	echo "[->] Test $i:"
	echo "[->] Test $i:" >> $2.txt
	mpirun -np 2 --hostfile host ./Paralelo $3 $4 $5 >> $2.txt
done
