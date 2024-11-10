#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int esPrimo(int num) {
	if (num <= 1) return 0;
	for (int i = 2; i * i <= num; i++) {
		if (num % i == 0) return 0;
	}
	return 1;
}

void inicializar(int **matriz, int fila, int columna) {
	srand(time(NULL));
	for (int i = 0; i < fila; i++) {
		for (int j = 0; j < columna; j++) {
			matriz[i][j] = rand() % 101;
		}
	}
}

void mostrarRand(int **matriz, int fila, int columna) {
	for (int i = 0; i < fila; i++) {
		for (int j = 0; j < columna; j++) {
			printf("%d ", matriz[i][j]);
		}
		printf("\n");
	}
}

void calcularRand(int **matriz, int fila, int columna) {
	int contador = 0;
	for (int i = 0; i < fila; i++) {
		for (int j = 0; j < columna; j++) {
			contador += esPrimo(matriz[i][j]);
		}
	}
	printf("Cantidad de números primos: %d\n", contador);
}

int main(int argc, char *argv[]) {

	int fila = atoi(argv[1]);
	int columna = atoi(argv[2]);

	int **matriz = (int **)malloc(fila * sizeof(int *));
	for (int i = 0; i < fila; i++) {
		matriz[i] = (int *)malloc(columna * sizeof(int));
	}

	inicializar(matriz, fila, columna);
	//mostrarRand(matriz, fila, columna);

	clock_t inicio, fin;
	double tiempo;
	inicio = clock();
	calcularRand(matriz, fila, columna);
	fin = clock();
	tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;
	printf("Tiempo: %f segundos\n", tiempo);

	return 0;
}
