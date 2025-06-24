#include <stdio.h>

int main(){

int calificacion;

    printf("Ingrese la calificacion: \n");
    scanf ("%d", &calificacion);

    if(calificacion >= 90){
        printf("Calificacion: A");
    }

    if(calificacion >=80 & calificacion < 90 ){
        printf("Calificacion: B");
    }

    if(calificacion >= 70 & calificacion < 80){
        printf("Calificacion: C");
    }

    if(calificacion >= 60 & calificacion < 70){
        printf("Calificacion: D");
    }

    if(calificacion < 60){
        printf("Calificacion: F");
    }

return 0;
}
