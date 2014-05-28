/*
 * usuario/dormilon.c
 *
 *  Juan Marín Vega
 *
 */

/*
 * Programa de usuario que solicita que lo pongan a dormir x segundos
 */

#include "servicios.h"

#define SEGUNDOS 0

int main(){
    int i, counter;
    counter = 3;
    printf("Dormilon: comienza\n");
    fijar_prio(40);
    while(counter>=0){
        i = dormir((unsigned int) SEGUNDOS);
        counter--;
    }
    
    if (!i)
        printf("proceso despertado con exito\n");
    
    printf("Dormilon: termina\n");
    return 0;
}
