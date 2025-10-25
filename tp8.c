#include <stdio.h>
#include <stdlib.h>

#define TAM 20

int cuantas_piezas(int);
int rectificable(float, float);
int rechazada(float, float);

void cargar_piezas(float *, int);
int contar(int (*)(float, float), float, float *, int);
float* reservar_memoria(int);
void segregar(int (*)(float, float), float, float *, int, float *);
void mostrar_piezas(char *, float *, int);

int main (void) {

    float diametros[TAM] = {0};
    int n;
    float max = 12;
    float min = 10;

    n = cuantas_piezas(TAM);

    cargar_piezas(diametros, n);

    int n_rectificar = contar(rectificable, max, diametros, n);
    float *p_rectificables = reservar_memoria(n_rectificar);
    segregar(rectificable, max, diametros, n, p_rectificables);

    int n_rechazar = contar(rechazada, min, diametros, n);
    float *p_rechazadas = reservar_memoria(n_rechazar);
    segregar(rechazada, min, diametros, n, p_rechazadas);

    mostrar_piezas("rectificables", p_rectificables, n_rectificar);
    mostrar_piezas("rechazadas", p_rechazadas, n_rechazar);

    return 0;
}

int cuantas_piezas(int max) {
    int n;

    do {
        printf("Cuantas piezas ingresara? ");
        scanf("%d", &n);
    } while (n < 0 || n > max);

    return n;
}

int rectificable(float diametro, float valor) {
    if (diametro > valor)
        return 1;
    else
        return 0;
}

int rechazada(float diametro, float valor) {
    if (diametro < valor)
        return 1;
    else
        return 0;
}

// Punto a)
void cargar_piezas(float *p, int n) {
    int i = 0;

    while (i < n) {
        float x;
        int ok;

        printf("Ingrese diametro de pieza %d: ", i);
        ok = scanf(" %f", &x);

        if (ok == 1) {
            if (x > 0) {
                *(p + i) = x;
                i++;
            } else {
                printf("Valor invalido (debe ser > 0). Reintente.\n");
            }
        } else {
            printf("Entrada invalida. Reintente.\n");
            scanf("%*s");
        }
    }
}


// Punto b)
int contar(int (*criterio)(float, float), float valor, float *p, int n) {
    int c = 0;

    for (int i = 0; i < n; i++) {

        if (criterio(*(p + i), valor)) {
            c++;}

    }

    return c;
}


// Punto c)
float* reservar_memoria(int n) {
    float *q;
    if (n <= 0) {
        return NULL;}

    q = (float*) malloc(n * sizeof(float));

    if (q == NULL) {
        printf("No se pudo reservar memoria.\n");
        return NULL;}

    return q;
}

// Punto d)
void segregar(int (*criterio)(float, float), float valor,
              float *p, int n, float *q) {
    int j = 0;
    for (int i = 0; i < n; i++) {
        float d = *(p + i);
        if (criterio(d, valor)) {
            *(q + j) = d;
            j++;
        }
    }
}


// Punto e)
void mostrar_piezas(char *mensaje, float *p, int n) {
    printf("Piezas %s:\n", mensaje);
    for (int i = 0; i < n; i++) {
        printf("Diametro: %.2f\n", *(p + i));
    }
}

