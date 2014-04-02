/*
 * usuario/simplon.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 * Programa de usuario que simplemente imprime un valor entero
 */

#include "servicios.h"

#define TOT_ITER 1000000000 /* ponga las que considere oportuno */

int main(){
	int i, alert;

	//for (i=0; i<TOT_ITER; i++)
	//	printf("simplon: i %d\n", i);
	printf("simplon: comienza\n");
    i = get_pid();
	printf("PID del simplon: %d\n", i);
    printf("Proceso largo en marcha\n");
    alert = 0;
	for( i = 0; i < TOT_ITER; i++){
        if(++alert == 100000000){
            alert = 0;
            printf("Simplon procesando\n");
            dormir(1);
        }
    }
    printf("simplon: termina\n");

	return 0;
}
