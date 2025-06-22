#include <stdio.h>

int main() {

    int CantidadDeEstudiantes, Nota;
    int TotalDeNotas = 0;
    int NotaMasAlta=0;
    int NotaMasBaja=100;

    // Filtro y pedido estudiantes

    do {

        printf ("ingrese la cantidad de estudiantes: \n");
        scanf("%d", &CantidadDeEstudiantes);

        if (CantidadDeEstudiantes <=0){
            printf("La cantidad de estudiantes debe ser un numero positivo \n");
        }

    } while (CantidadDeEstudiantes <=0);

    //Filtro y pedido de notas

    for(int i=1; i <= CantidadDeEstudiantes; i++){
        do {
        printf("Ingrese las notas del estudiante numero %d: \n",i);
        scanf("%d", &Nota);

            if(Nota < 0 || Nota > 100){
                printf("La nota debe estar comprendida entre 0 y 100 \n");
            }

        } while (Nota < 0 || Nota > 100);

     //Calcular resultados

        TotalDeNotas += Nota;

        if(Nota > NotaMasAlta){
           NotaMasAlta = Nota;
        }

         if(Nota < NotaMasBaja){
           NotaMasBaja = Nota;
        }

    }

    float promedio = TotalDeNotas / CantidadDeEstudiantes;
    printf("El promedio de las notas es %2.f \n" , promedio);

    printf("La nota mas alta es: %d \n" , NotaMasAlta);
    printf("La nota mas baja es: %d \n" , NotaMasBaja);

      return 0;

}
