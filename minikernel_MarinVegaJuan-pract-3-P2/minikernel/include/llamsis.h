/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 8

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define GET_PID 3
#define DORMIR 4
#define FIJAR_PRIO 5
#define GET_PPID 6
#define ESPERA 7

#endif /* _LLAMSIS_H */

