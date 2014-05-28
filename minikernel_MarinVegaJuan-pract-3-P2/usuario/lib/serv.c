/*
 *  usuario/lib/serv.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene las definiciones de las funciones de interfaz
 * a las llamadas al sistema. Usa la funcion de apoyo llamsis
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#include "llamsis.h"
#include "servicios.h"

/* Funci�n del m�dulo "misc" que prepara el c�digo de la llamada
   (en el registro 0), los par�metros (en registros 1, 2, ...), realiza la
   instruccion de llamada al sistema  y devuelve el resultado 
   (que obtiene del registro 0) */

int llamsis(int llamada, int nargs, ... /* args */);


/*
 *
 * Funciones interfaz a las llamadas al sistema
 *
 */


int crear_proceso(char *prog){
	return llamsis(CREAR_PROCESO, 1, (long)prog);
}
int terminar_proceso(){
	return llamsis(TERMINAR_PROCESO, 0);
}
int escribir(char *texto, unsigned int longi){
	return llamsis(ESCRIBIR, 2, (long)texto, (long)longi);
}
int get_pid(){
        return llamsis(GET_PID, 0); /* no recibe argumentos de entrada*/
}
int dormir(unsigned int segundos){
        /*  pasamos segundos como long ya que las demas hacen lo mismo */
        return llamsis(DORMIR, 1, (long)segundos);
}
int fijar_prio(unsigned int prio){
    return llamsis(FIJAR_PRIO, 1, (long)prio);
}
int get_ppid(){
    return llamsis(GET_PPID, 0);
}
int espera(){
    return llamsis(ESPERA, 0);
}