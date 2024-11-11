#/bin/sh

# $1: Numero de pruebas
# $2: Nombre del archivo
# $3: Numero de filas
# $4: Numero de columnas
# $5: Numero de filas/worker

# Compilacion
gcc MatrizSecuencial.c -o Secuencial
mpicc MatrizParalelo.c -o Paralelo
echo "[->] Compilacion Exitosa"
mkdir test

# Version secuencial
echo "[->] Calculo Secuencial"
echo "[->] Calculo Secuencial" >> test/$2.txt
for ((i = 1; i < ($1+1); i++)); do
	echo "[->] Test $i:"
	echo "[->] Test $i:" >> test/$2.txt
	./Secuencial $3 $4 >> test/$2.txt
done

# Version paralela
echo "[->] Calculo Paralelo"
echo "[->] Calculo Paralelo" >> test/$2.txt
for ((i = 1; i < ($1+1); i++)); do
	echo "[->] Test $i:"
	echo "[->] Test $i:" >> test/$2.txt
	mpirun -np 2 --hostfile host ./Paralelo $3 $4 $5 >> test/$2.txt
done
