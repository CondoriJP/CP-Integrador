#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Definición de constantes para los tags de los mensajes MPI
#define TAREA 1
#define RESULTADO 2
#define FIN 3
#define ULTIMA 4

// Función para determinar si un número es primo
int esPrimo(unsigned char num) {
	if (num <= 1) return 0;  // Los números menores o iguales a 1 no son primos
	for (int i = 2; i * i <= num; i++) {  // Solo es necesario iterar hasta la raíz cuadrada de num
		if (num % i == 0) return 0;  // Si es divisible, no es primo
	}
	return 1;  // Si no fue divisible por ningún número, es primo
}

// Función para inicializar la matriz con valores aleatorios entre 0 y 100
void inicializar(unsigned char *matriz, int filas, int columnas) {
	srand(time(NULL));  // Inicializa la semilla para la generación de números aleatorios
	for (int i = 0; i < (filas * columnas); i++) {
		matriz[i] = rand() % 101;  // Asigna un valor aleatorio entre 0 y 100 a cada elemento
	}
}

// Función para mostrar la matriz en la consola
void mostrar_matriz(unsigned char *matriz, int filas, int columnas) {
	for (int i = 0; i < filas; i++) {
		for (int j = 0; j < columnas; j++) {
			printf("%d ", matriz[i * columnas + j]);  // Imprime cada elemento de la matriz
		}
		printf("\n");
	}
}

// Función para mostrar una fila de la matriz
void mostrar_fila(unsigned char *vector, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d ", vector[i]);  // Imprime cada número de la fila
	}
}

int main(int argc, char **argv) {
	int rank, size;  // rank: ID del proceso, size: número total de procesos
	MPI_Init(&argc, &argv);  // Inicializa MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Obtiene el rank del proceso actual
	MPI_Comm_size(MPI_COMM_WORLD, &size);  // Obtiene el número total de procesos

	// Verifica si hay al menos 2 procesos (master + al menos un worker)
	if (size < 2) {
		printf("[!] Se necesitan al menos 2 procesos\n");
		MPI_Abort(MPI_COMM_WORLD, 1);  // Termina el programa si no hay suficientes procesos
	}

	// Lee las dimensiones de la matriz y la cantidad de filas por worker desde los argumentos
	int filas = atoi(argv[1]);
	int columnas = atoi(argv[2]);
	int cantFilas = atoi(argv[3]);

	if (rank == 0) {
		// Proceso master (rank == 0)
		printf("[+] Master: cantidad de Workers %d\n", size - 1);
		printf("[+] Master: matriz de %d x %d a %d fila/worker\n", filas, columnas, cantFilas);

		// Variables para medir el tiempo y almacenar los resultados
		double inicio, fin, acumulado = 0;
		double *inicioW = (double *)malloc((size) * sizeof(double));
		double *finW = (double *)malloc((size) * sizeof(double));
		int primos_totales = 0;
		int primos_locales = 0;
		int worker = 1;
		int workerFIN = 0;
		int flag = 0;
		int resto = filas % cantFilas;  // Resto de filas que no encajan en bloques completos
		MPI_Status status;  // Variable para almacenar el estado de los mensajes MPI
		unsigned char *matriz = (unsigned char *)malloc(filas * columnas * sizeof(unsigned char));
		unsigned char *fila = (unsigned char *)malloc(columnas * cantFilas * sizeof(unsigned char));

		// Inicializa los tiempos para los Workers
		for (int i = 0; i < size; i++) {
			inicioW[i] = 0;
			finW[i] = 0;
		}

		// Inicializa la matriz con valores aleatorios
		inicializar(matriz, filas, columnas);
		//mostrar_matriz(matriz, filas, columnas);  // Opcional: muestra la matriz completa

		// Medir el tiempo de inicio
		inicio = MPI_Wtime();

		// Enviar filas de la matriz a los Workers
		for (int i = 0; i < (filas * columnas); i += (cantFilas * columnas)) {
			if (worker < size) {
				if (resto != 0 && i == (filas - resto) * columnas) {
					// Si hay filas restantes (resto), enviarlas al último Worker
					MPI_Send(&matriz[i], columnas * resto, MPI_UNSIGNED_CHAR, worker, ULTIMA, MPI_COMM_WORLD);
					MPI_Send(&resto, 1, MPI_INT, worker, ULTIMA, MPI_COMM_WORLD);
					break;
				}
				// Enviar bloques completos de filas a los Workers
				inicioW[worker] = MPI_Wtime();  // Tiempo de inicio para el Worker
				MPI_Send(&matriz[i], columnas * cantFilas, MPI_UNSIGNED_CHAR, worker, TAREA, MPI_COMM_WORLD);
				worker++;
			} else {
				// El Master sigue trabajando mientras espera resultados de los Workers
				MPI_Iprobe(MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &flag, &status);
				if (!flag && (resto == 0 || i < (filas - resto) * columnas)) {
					// Si no hay resultados, el Master procesa las filas restantes
					inicioW[0] = MPI_Wtime();
					fila = &matriz[i];
					primos_locales = 0;
					for (int i = 0; i < columnas * cantFilas; i++) {
						primos_locales += esPrimo(fila[i]);  // Cuenta los primos en la fila
					}
					primos_totales += primos_locales;  // Suma los primos encontrados
					finW[0] = MPI_Wtime();  // Tiempo de finalización del Master
					acumulado += finW[0] - inicioW[0];  // Acumula el tiempo de procesamiento
				} else {
					// Recibe el resultado de un Worker y asigna una nueva fila
					int primos;
					MPI_Recv(&primos, 1, MPI_INT, MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &status);
					primos_totales += primos;  // Suma los primos encontrados por el Worker

					// Verificar si se está procesando la última fila (con resto)
					if (resto != 0 && i == (filas - resto) * columnas) {
						MPI_Send(&matriz[i], columnas * resto, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, ULTIMA, MPI_COMM_WORLD);
						MPI_Send(&resto, 1, MPI_INT, status.MPI_SOURCE, ULTIMA, MPI_COMM_WORLD);
						break;
					}
					// Reenviar una fila al Worker que terminó su tarea
					MPI_Send(&matriz[i], columnas * cantFilas, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, TAREA, MPI_COMM_WORLD);
				}
			}
		}

		// Enviar mensaje de FIN a los Workers restantes que no han recibido trabajo
		if (worker != size) {
			for (int i = worker; i < size; i++) {
				workerFIN++;
				MPI_Send(NULL, 0, MPI_INT, i, FIN, MPI_COMM_WORLD);
			}
		}

		// Recibir resultados finales de los Workers
		for (int i = 1; i < (size - workerFIN); i++) {
			int primos;
			MPI_Recv(&primos, 1, MPI_INT, MPI_ANY_SOURCE, RESULTADO, MPI_COMM_WORLD, &status);
			finW[status.MPI_SOURCE] = MPI_Wtime();  // Tiempo de finalización del Worker
			printf("[>] Worker %d (%f seg Comuni-Proces)\n", status.MPI_SOURCE, finW[status.MPI_SOURCE] - inicioW[status.MPI_SOURCE]);
			primos_totales += primos;
			// Enviar FIN a cada Worker para terminar su ejecución
			MPI_Send(NULL, 0, MPI_INT, status.MPI_SOURCE, FIN, MPI_COMM_WORLD);
		}

		// Medir el tiempo de finalización del Master
		fin = MPI_Wtime();
		printf("[>] Número total de primos: %d\n", primos_totales);
		free(matriz);  // Liberar la memoria de la matriz
		free(fila);  // Liberar la memoria de la fila
		printf("[*] Master terminado (%f seg Proces) (%f seg Glob)\n", acumulado, fin - inicio);

	} else {
		// Código para los Workers (rank > 0)
		double inicio, fin, acumulado = 0;
		MPI_Status status;
		unsigned char *fila = (unsigned char *)malloc(columnas * cantFilas * sizeof(unsigned char));
		int primos_locales;

		// Bucle que mantiene a los Workers procesando hasta recibir FIN
		while (1) {
			MPI_Recv(fila, columnas * cantFilas, MPI_UNSIGNED_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_TAG == TAREA) {
				// Cuando recibe una tarea, cuenta los primos en la fila
				inicio = MPI_Wtime();
				primos_locales = 0;
				for (int i = 0; i < columnas * cantFilas; i++) {
					primos_locales += esPrimo(fila[i]);
				}
				fin = MPI_Wtime();
				acumulado += fin - inicio;  // Acumula el tiempo de procesamiento
				MPI_Send(&primos_locales, 1, MPI_INT, 0, RESULTADO, MPI_COMM_WORLD);  // Enviar el resultado al Master
			} else if (status.MPI_TAG == ULTIMA) {
				// Si recibe la última parte de filas (resto), procesa de forma similar
				int resto;
				MPI_Recv(&resto, 1, MPI_INT, 0, ULTIMA, MPI_COMM_WORLD, &status);
				inicio = MPI_Wtime();
				primos_locales = 0;
				for (int i = 0; i < columnas * resto; i++) {
					primos_locales += esPrimo(fila[i]);
				}
				fin = MPI_Wtime();
				acumulado += fin - inicio;
				MPI_Send(&primos_locales, 1, MPI_INT, 0, RESULTADO, MPI_COMM_WORLD);
			} else if (status.MPI_TAG == FIN) {
				// Recibe el mensaje de FIN y termina su ejecución
				printf("[*] Worker %d terminado (%f seg Proces)\n", rank, acumulado);
				break;  // El Worker termina su ejecución
			}
		}
		free(fila);  // Liberar la memoria de la fila
	}

	// Finalizar la ejecución de MPI
	MPI_Finalize();
	return 0;
}
