#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAREA 1
#define RESULTADO 2
#define FIN 3
#define ULTIMA 4

int esPrimo(int num) {
	if (num <= 1) return 0;
	for (int i = 2; i * i <= num; i++) {
		if (num % i == 0) return 0;
	}
	return 1;
}

void inicializar(int *matriz, int filas, int columnas) {
	srand(time(NULL));
	for (int i = 0; i < (filas*columnas); i++) {
		matriz[i] = rand() % 101;  // Valores aleatorios entre 0 y 100
	}
}

void mostrar_matriz(int *matriz, int filas, int columnas) {
	for (int i = 0; i < filas; i++) {
		for (int j = 0; j < columnas; j++) {
			printf("%d ", matriz[i*columnas + j]);
		}
		printf("\n");
	}
}

void mostrar_fila(int *vector, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d ", vector[i]);
	}
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

	int filas = atoi(argv[1]);
	int columnas = atoi(argv[2]);
	int cantFilas = atoi(argv[3]);

	if (rank == 0) {
		// Master
		printf("[+] Master: cantidad de Workers %d\n", size - 1);
		printf("[+] Master: matriz de %d x %d a %d fila/worker\n", filas, columnas, cantFilas);
		double inicio, fin;
		int primos_totales = 0;
		int worker = 1;
		int resto = filas % cantFilas;
		MPI_Status status;
		int *matriz = (int *)malloc(filas * columnas * sizeof(int));
		// Inicializar matriz
		inicializar(matriz, filas, columnas);
		//mostrar_matriz(matriz, filas, columnas);
		inicio = MPI_Wtime();

		// Enviar filas a los Workers
		for (int i = 0; i < (filas*columnas); i+=(cantFilas*columnas)) {
			if (worker < size) {
				// Enviar las primeras filas a un worker
				MPI_Send(&matriz[i] , columnas*cantFilas, MPI_INT, worker, TAREA, MPI_COMM_WORLD);
				worker++;
			} else {
				// Recibir resultado de un worker y asignar una nueva fila
				int primos;
				MPI_Recv(&primos, 1, MPI_INT, MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &status);
				primos_totales += primos;

				// Verificar si es la ultima fila
				if (resto != 0 && i == (filas - resto) * columnas) {
					// Enviar la ultima fila a un worker
					MPI_Send(&matriz[i], columnas * resto, MPI_INT, status.MPI_SOURCE, ULTIMA, MPI_COMM_WORLD);
					// Enviar el tamaño de la ultima fila (resto)
					MPI_Send(&resto, 1, MPI_INT, status.MPI_SOURCE, ULTIMA, MPI_COMM_WORLD);
					break;
				}
				// Reenviar una fila al worker que terminó
				MPI_Send(&matriz[i], columnas*cantFilas, MPI_INT, status.MPI_SOURCE, TAREA, MPI_COMM_WORLD);
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
		free(matriz);
		printf("[*] Master terminado\n\n");

	} else {
		// Workers
		MPI_Status status;
		int *fila = (int *)malloc(columnas * cantFilas * sizeof(int));
		int primos_locales;
		while (1) {
			MPI_Recv(fila, columnas * cantFilas, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_TAG == TAREA) {
				primos_locales = 0;
				for (int i = 0; i < columnas * cantFilas; i++) {
					primos_locales += esPrimo(fila[i]);
				}
				//printf("[>] Worker %d: %d primos en esta fila: ", rank, primos_locales);
				//mostrar_fila(fila, columnas * cantFilas);
				//printf("\n");
				MPI_Send(&primos_locales, 1, MPI_INT, 0, RESULTADO, MPI_COMM_WORLD);

			} else if (status.MPI_TAG == ULTIMA) {
				int resto;
				MPI_Recv(&resto, 1, MPI_INT, 0, ULTIMA, MPI_COMM_WORLD, &status);
				primos_locales = 0;
				for (int i = 0; i < columnas * resto; i++) {
					primos_locales += esPrimo(fila[i]);
				}
				//printf("[>] Worker %d: %d primos en esta fila: ", rank, primos_locales);
				//mostrar_fila(fila, columnas * resto);
				//printf("\n");
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
