//

#include <stdio.h>
#include <stdlib.h>

#define N_MAX 10
#define M_MAX 10

struct matrix {
  int filas;
  int columnas;
  float mat[N_MAX][M_MAX];
};

typedef struct matrix matrix_t;

void cargar_matriz(matrix_t *, char *);
void sumar_matrices(matrix_t, matrix_t, matrix_t *);
matrix_t sumar_matrices_r(matrix_t, matrix_t);
void imprimir_matriz(matrix_t);
void guardar_matriz(matrix_t, char *);

//0 = OK, 1 = ERROR 
int g_error = 0;

int main(void){
    matrix_t A = {0}, B = {0};
    matrix_t C = {0};

    cargar_matriz(&A, "mat_A.txt");
    if (g_error) return 1;

    cargar_matriz(&B, "mat_B.txt");
    if (g_error) return 1;

    printf("Matriz A:\n");
    imprimir_matriz(A);
    printf("\n");

    printf("Matriz B:\n");
    imprimir_matriz(B);
    printf("\n");

    sumar_matrices(A, B, &C);
    if (g_error) return 1;

    //C = sumar_matrices_r(A, B);
    printf("Matriz C = A + B:\n");
    imprimir_matriz(C);
    printf("\n");

    guardar_matriz(C, "mat_C.txt");
    if (g_error) return 1;

    return 0;
}

void cargar_matriz(matrix_t *p, char *file) {
    FILE *fPtr = fopen(file, "r");
    if (fPtr == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'\n", file);
        g_error = 1;
        return;
  }

  // Leer filas y columnas 
    if (fscanf(fPtr, "%d %d", &p->filas, &p->columnas) != 2) {
        printf("Error: formato inválido en la primera línea de '%s'\n", file);
        fclose(fPtr);
        g_error = 1;
        return;
  }

  // Validar 
    if (p->filas <= 0 || p->filas > N_MAX || p->columnas <= 0 || p->columnas > M_MAX) {
        printf("Error: dimensiones fuera de rango en '%s' (max %dx%d)\n",
           file, N_MAX, M_MAX);
        fclose(fPtr);
        g_error = 1;
        return;
  }

  // Leer los elementos 
    for (int i = 0; i < p->filas; i++) {
        for (int j = 0; j < p->columnas; j++) {
            if (fscanf(fPtr, "%f", &p->mat[i][j]) != 1) {
                printf("Error: datos insuficientes al leer '%s' en (%d,%d)\n",
                    file, i, j);
                fclose(fPtr);
                g_error = 1;
                return;
      }
    }
  }

    fclose(fPtr);
}

void imprimir_matriz(matrix_t m) {
  for (int i = 0; i < m.filas; i++) {
    for (int j = 0; j < m.columnas; j++)
      printf("%9.2f", m.mat[i][j]);
    printf("\n");
  }
}

void sumar_matrices(matrix_t A, matrix_t B, matrix_t *pC) {
  if (A.filas != B.filas || A.columnas != B.columnas) {
    printf("Error: las matrices tienen dimensiones distintas, no se pueden sumar\n");
    g_error = 1;
    return;
  }

  pC->filas = A.filas;
  pC->columnas = A.columnas;

  for (int i = 0; i < A.filas; i++) {
    for (int j = 0; j < A.columnas; j++) {
      pC->mat[i][j] = A.mat[i][j] + B.mat[i][j];
    }
  }
}

void guardar_matriz(matrix_t m, char *file) {
  FILE *fPtr = fopen(file, "w");
  if (fPtr == NULL) {
    printf("Error: no se pudo abrir '%s' para escritura\n", file);
    g_error = 1;
    return;
  }

  fprintf(fPtr, "%d %d\n", m.filas, m.columnas);

  for (int i = 0; i < m.filas; i++) {
    for (int j = 0; j < m.columnas; j++) {
      fprintf(fPtr, "%f", m.mat[i][j]);
      if (j == m.columnas - 1) {
        fprintf(fPtr, "\n");
      } else {
        fprintf(fPtr, " ");
      }
    }
  }

  fclose(fPtr);
}
