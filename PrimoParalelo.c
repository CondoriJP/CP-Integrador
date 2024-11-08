#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAREA 1
#define RESULTADO 2
#define FIN 3

int esPrimo(int num) {
	if (num <= 1) return 0;
	for (int i = 2; i * i <= num; i++) {
		if (num % i == 0) return 0;
	}
	return 1;
}

void inicializar(int **matriz, int filas, int columnas) {
	srand(time(NULL));
	for (int i = 0; i < filas; i++) {
		for (int j = 0; j < columnas; j++) {
			matriz[i][j] = rand() % 101;  // Valores aleatorios entre 0 y 100
		}
	}
}

void mostrar_matriz(int **matriz, int filas, int columnas) {
	printf("\n");
	for (int i = 0; i < filas; i++) {
		for (int j = 0; j < columnas; j++) {
			printf("%d ", matriz[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void mostrar_fila(int *vector, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d ", vector[i]);
	}
	printf("\n");
}

int main(int argc, char **argv) {
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (size < 2) {
		printf("[!] Se necesitan al menos 2 procesos\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	int filas = 5000;
	int columnas = 5000;

	if (rank == 0) {
		// Master
		printf("[+] Master: cantidad de Workers %d\n", size - 1);
		double inicio, fin;
		int primos_totales = 0;
		int worker = 1;
		MPI_Status status;
		int **matriz = (int **)malloc(filas * sizeof(int *));
		for (int i = 0; i < filas; i++) {
			matriz[i] = (int *)malloc(columnas * sizeof(int));
		}
		inicializar(matriz, filas, columnas);
		//mostrar_matriz(matriz, filas, columnas);
		inicio = MPI_Wtime();

		// Enviar filas a los Workers
		for (int i = 0; i < filas; i++) {
			if (worker < size) {
				// Enviar fila a un worker
				MPI_Send(&matriz[i][0], columnas, MPI_INT, worker, TAREA, MPI_COMM_WORLD);
				worker++;
			} else {
				// Recibir resultado de un worker y asignar una nueva fila
				int primos;
				MPI_Recv(&primos, 1, MPI_INT, MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &status);
				primos_totales += primos;

				// Reenviar una fila al worker que terminó
				MPI_Send(&matriz[i][0], columnas, MPI_INT, status.MPI_SOURCE, TAREA, MPI_COMM_WORLD);
			}
		}

		// Recibir respuestas de los Workers restantes
		for (int i = 1; i < size; i++) {
			int primos;
			MPI_Recv(&primos, 1, MPI_INT, MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &status);
			primos_totales += primos;

			// Enviar FIN a cada worker para terminar su ejecución
			MPI_Send(NULL, 0, MPI_INT, status.MPI_SOURCE, FIN, MPI_COMM_WORLD);
		}

		fin = MPI_Wtime();
		printf("[>] Número total de primos: %d\n", primos_totales);
		printf("[>] Tiempo: %f segundos\n", fin - inicio);

		// Liberar la matriz
		for (int i = 0; i < filas; i++) {
			free(matriz[i]);
		}
		free(matriz);
		printf("[*] Master terminado\n\n");

	} else {
		// Workers
		MPI_Status status;
		int *fila = (int *)malloc(columnas * sizeof(int));
		int primos_locales;
		while (1) {
			MPI_Recv(fila, columnas, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_TAG == TAREA) {
				primos_locales = 0;
				for (int i = 0; i < columnas; i++) {
					primos_locales += esPrimo(fila[i]);
				}
				//printf("[>] Worker %d: %d primos en esta fila\n", rank, primos_locales);
				MPI_Send(&primos_locales, 1, MPI_INT, 0, RESULTADO, MPI_COMM_WORLD);
			} else if (status.MPI_TAG == FIN) {
				printf("[*] Worker %d terminado\n", rank);
				break;  // El worker termina
			}
		}
		free(fila);
	}
	MPI_Finalize();
	return 0;
}
