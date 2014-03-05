/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
    p_proc_actual->estado=EJECUCION;
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}

/*
 * Funcion auxiliar que pone un proceso en bloqueado
 * y hace un cambio de contexto
 *
 */
static void bloquear(lista_BCPs * lista){
    BCP * p_proc_anterior;
    int nivel;
   
    /* bloqueamos el proceso y apuntamos a el */
    p_proc_actual->estado=BLOQUEADO;
    p_proc_anterior=p_proc_actual;

    /* detenemos interrupciones */
	nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
    /* lo elimino de lista listos y lo inserto en lista_dormidos */
    
    eliminar_elem(&lista_listos, p_proc_anterior);
    insertar_ultimo(&lista_dormidos, p_proc_anterior);

    /*  volvemos a poner interrupciones como antes */
	fijar_nivel_int(nivel);

    /* llamamos al planificador para recuperar el nuevo proceso */
    p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR BLOQUEO: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);
    p_proc_actual->estado=EJECUCION;
    cambio_contexto(&(p_proc_anterior->contexto_regs), 
            &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}


/*
 * Funcion auxiliar que pone desbloquea de la lista que recibeo 
 * y lo inserta en la lista listos
 *
 */
static void desbloquear(BCP * proc, lista_BCPs * lista){
	BCP *paux=lista->primero;
    BCP *p_proc_listo = NULL;
    /*  primero miramos si el que hay que desbloquear es el primero */
	if (paux==proc){
        p_proc_listo = proc;
		eliminar_primero(lista);
    }else {
        /* recorremos los procesos hasta encontrar el que queremos
         * o llegar al final
         */
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);

        /* Ahora comprobamos si paux no es nulo */
		if (paux) {
            p_proc_listo = proc;
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
    /* Solo insertamos el proceso de nuevo a listo si esta en la
     * lista que nos han pasado
     * */
    if(p_proc_listo){
        p_proc_listo->estado=LISTO;
        insertar_ultimo(&lista_listos, p_proc_listo);
    }
    return;
}


/*
 * Funcion auxiliar que acutaliza los ticks de los procesos dormidos
 * cuando detecta que uno no tiene ticks pendientes lo envia a desbloquear
 *
 */

static void ajustar_dormidos(){
	BCP *paux=lista_dormidos.primero;

//    for( ; paux != NULL; paux=paux->siguiente){
//        paux->nticks -= 1;
//        if(paux->nticks == 0){
//	        printk("-> DEBLOQUEANDO PROC: %d\n", paux->id);
//            desbloquear(paux, &lista_dormidos);
//        }
//    }

    while(paux){
       // printk("Proceso id: %i, ticks remaining: %i\n", paux->id, paux->nticks);
        paux->nticks--;
        if(paux->nticks == 0){
	        printk("-> DESPERTANDO PROC: %d\n", paux->id);
            desbloquear(paux, &lista_dormidos);
        }
        paux = paux->siguiente;
    }
    return;
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){

	printk("-> TRATANDO INT. DE TERMINAL %c\n", leer_puerto(DIR_TERMINAL));

        return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	printk("-> TRATANDO INT. DE RELOJ\n");
    ajustar_dormidos();

        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;
        // leemos la posicion 0 del registro
        // que es donde esta el numero del servicio
        // a ejecutar
	nserv=leer_registro(0);
        // si el numero esta dentro del rango de
        // los registros definidos
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
        
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	printk("-> TRATANDO INT. SW\n");

	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema get_pid. Muestra el id del 
 * proceso actual
 */
int sis_get_pid(){
    printk("-> A DINS DE SIS_GET_PID\n");
    return p_proc_actual->id;

}

/*
 * Tratamiento de llamada al sistema dormir. Pone el proceso actual 
 * a dormir y pasa al siguiente proceso
 */
int sis_dormir(){

    unsigned int segundos;
    segundos=(unsigned int)leer_registro(1);
    printk("-> PROC %d A DORMIR %d SEGUNDOS\n",p_proc_actual->id, segundos);
    
    /* insertamos el numero de ticks */
    p_proc_actual->nticks = segundos * TICK;
    
    /* solicitamos poner el proceso a dormir */
    bloquear(&lista_dormidos);
    return 0;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}

/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */
	iniciar_tabla_proc();

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
    p_proc_actual->estado = EJECUCION;
                    /* proceso que dejo de ejecutar, y proceso que paso a ejecutar*/
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
