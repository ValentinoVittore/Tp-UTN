#include <stdio.h>
#define TAM 5

void cargar_arreglo(int codigo[], float precio[], int n);
void imprimir_tabla(int codigo[], float precio[], int n);
void mayor_precio(int codigo[], float precio[], int n);
void menor_precio(int codigo[], float precio[], int n);

void cargar_arreglo (int codigo[], float precio[], int n){
    for(int i = 0; i < n; i++){
            do{
        printf("ingrese el codigo: \n");
        scanf ("%d",&codigo[i]);
                if (codigo[i] < 1 || codigo[i] > 999999999) {
                puts("Error. El código de barras debe estar entre 1 y 999999999\n");
            }
    }while(codigo[i] < 1 || codigo[i] > 999999999);
    
    do{
        printf("ingrese el precio: \n");
        scanf ("%f",&precio[i]);
                if (precio[i]<0) {
                puts("Error. El precio debe ser un numero positivo\n");
            }
    }while(precio[i]<0);
}
}

void imprimir_tabla(int codigo[], float precio[], int n){
    printf("\n%-12s %12s\n", "Código", "Precio");
    for(int i = 0; i < n; i++){
            printf ("%-12d %12.2f\n", codigo[i], precio[i]);
    }
}

void mayor_precio(int codigo[], float precio[], int n){
    float PrecioMax = 0;
    int Posicion = 0;
    for(int i = 0; i < n; i++){
        if (PrecioMax < precio[i]){
            PrecioMax = precio[i];
            Posicion=i;
        }
    }
    printf("Más caro: [%d] %.2f \n", codigo[Posicion], precio[Posicion]);
}

void menor_precio(int codigo[], float precio[], int n){
    float PrecioMin = precio[0];
    int Posicion = 0;
    for(int i = 1; i < n; i++){
        if (PrecioMin > precio[i]){
            PrecioMin = precio[i];
            Posicion = i;
        }
    }
    printf("Más barato: [%d] %.2f \n", codigo[Posicion], precio[Posicion]);
}


int main(void) {
    
    int codigo[TAM];
    float precio[TAM];

    cargar_arreglo(codigo, precio, TAM);
    imprimir_tabla(codigo, precio, TAM);
    mayor_precio(codigo, precio, TAM);
    menor_precio(codigo, precio, TAM);
    
    return 0;
}
