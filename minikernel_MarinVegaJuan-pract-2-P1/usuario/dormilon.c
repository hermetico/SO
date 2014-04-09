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

#define SEGUNDOS 1

int main(){
    int i;

    printf("Dormilon: comienza\n");
    i = dormir((unsigned int) SEGUNDOS);
    
    if (!i)
        printf("proceso despertado con exito\n");
    
    printf("Dormilon: termina\n");
    return 0;
}
