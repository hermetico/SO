/*
 * usuario/init.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/* Programa inicial que arranca el S.O. S�lo se ejecutar�n los programas
   que se incluyan aqu�, que, evidentemente, pueden ejecutar otros
   programas...
*/

#include "servicios.h"

int main(){

	printf("init: comienza\n");

//    fijar_prio(25);
//    if (crear_proceso("dormilon")<0)
//               printf("Error creando dormilon\n");
//
//    fijar_prio(30);
//    if (crear_proceso("simplon")<0)
//                printf("Error creando simplon\n");
//	/* Este programa causa una excepci�n */
//	if (crear_proceso("excep_arit")<0)
//		printf("Error creando excep_arit\n");
//    fijar_prio(10); 
//    /* Este programa solicita que lo pongan a dormir */
//	if (crear_proceso("dormilon")<0)
//                printf("Error creando dormilon\n");
//
	/* Este programa crea otro proceso que ejecuta simplon a
	   una excepci�n */
	if (crear_proceso("excep_mem")<0)
		printf("Error creando excep_mem\n");

//	fijar_prio(50);
//	/* No existe: debe fallar */
//	if (crear_proceso("noexiste")<0)
//		printf("Error creando noexiste\n");
//
    espera();
    printf("init: termina\n");
	return 0; 
}
