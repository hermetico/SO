/*
 * usuario/simplon.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 * Programa de usuario que simplemente imprime un valor entero
 */

#include "servicios.h"

#define TOT_ITER 10 /* ponga las que considere oportuno */

int main(){
	int i, alert;

	//for (i=0; i<TOT_ITER; i++)
	//	printf("simplon: i %d\n", i);
	printf("simplon: comienza\n");
    i = get_ppid();
	printf("PID del padre del simplon: %d\n", i);
    printf("Proceso largo en marcha\n");
    alert = 0;
	for( i = 0; i < TOT_ITER; i++){
        if(++alert == 100000000){
            alert = 0;
            printf("Simplon a dormir 1 segundo\n");
            dormir(1);
            printf("Desperando simplon, proceso largo en marcha\n");
        }
    }
	if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");
    
	if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");
	
    if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");
    
    espera();

	if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");
    
	if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");
	
    if (crear_proceso("dormilon")<0)
		printf("Error creando dormilon\n");

    espera();
    printf("simplon: termina\n");

	return 0;
}
