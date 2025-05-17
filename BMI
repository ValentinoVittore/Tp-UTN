#include <stdio.h>

int main(){

float peso, altura, BMI;

printf("Ingrese el peso en kg: ");
    scanf("%f", &peso);

printf("Ingrese la altura en metros: ");
    scanf("%f", &altura);

BMI = peso / (altura * altura);
printf("\nSu indice de masa corporal es: %.2f\n", BMI);

    printf("\nIndice | Condicion\n");
    printf("------------------------------\n");

    if (BMI < 18.5) {
        printf("<18.5 | Bajo peso\n");
    } else if (BMI < 25.0) {
        printf("18.5 - 24.9 | Normal\n");
    } else if (BMI < 30.0) {
        printf("25.0 - 29.9 | Sobrepeso\n");
    } else {
        printf(">=30.0 | Obesidad\n");
    }

return 0;
}
