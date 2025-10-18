#include <stdio.h>
#include <math.h>
#define PI 3.141592653589793

float calcularAreaRectangulo (float longitud_rectangulo, float altura_rectangulo){
    return longitud_rectangulo * altura_rectangulo;
}

float calcularPerimetroRectangulo (float longitud_rectangulo, float altura_rectangulo){
    return 2*(longitud_rectangulo+altura_rectangulo);
}

float calcularDiagonalRectangulo (float longitud_rectangulo, float altura_rectangulo){
    return sqrtf(longitud_rectangulo*longitud_rectangulo + altura_rectangulo*altura_rectangulo);
}

float calcularAreaCirculo (float radio_circulo){
    return PI*radio_circulo*radio_circulo;
}

float calcularPerimetroCirculo (float radio_circulo){
    return 2*PI*radio_circulo;
}

void imprimirResultados (float area_calculada, float perimetro_calculado){
    
    printf("El area calculada es: %.2f\n",area_calculada);
    printf("El perimetro calculado es: %.2f\n",perimetro_calculado); 
}

int main (void) {

float longitud, altura, radio, area, perimetro;
int opcion;

printf("Ingrese la figura que desea calcular (1: rectangulo, 2: circulo):\n");
do {
    scanf("%d", &opcion);
}while(opcion < 1 || opcion > 2);

switch(opcion){
    case 1:
        printf ("Opcion de rectangulo seleccionada \n");
        printf ("Ingrese la longitud del rectangulo: \n");
        scanf("%f", &longitud);
        printf ("Ingrese la altura del rectangulo: \n");
        scanf("%f", &altura);

        area = calcularAreaRectangulo(longitud, altura);
        perimetro = calcularPerimetroRectangulo(longitud, altura);
        imprimirResultados(area,perimetro);
        printf("Diagonal: %.2f\n", calcularDiagonalRectangulo(longitud, altura));
        break;

    case 2:
        printf ("Opcion de circulo seleccionada \n");
        printf ("Ingrese el radio del circulo: \n");
        scanf("%f", &radio);

        area = calcularAreaCirculo(radio);
        perimetro = calcularPerimetroCirculo(radio);
        imprimirResultados(area, perimetro);
        break;
    default:
        break;
}

return 0;
}
