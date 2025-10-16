#include <stdio.h>

int main(){

int calificacion;

    printf("Ingrese la calificacion: \n");
    scanf ("%d", &calificacion);

    if(calificacion >= 90){
        printf("Calificacion: A");
    }
    else if(calificacion >=80 && calificacion < 90 ){
        printf("Calificacion: B");
    }

    else if(calificacion >= 70 && calificacion < 80){
        printf("Calificacion: C");
    }

    else if(calificacion >= 60 && calificacion < 70){
        printf("Calificacion: D");
    }

   else{
        printf("Calificacion: F");
    }
return 0;
}
