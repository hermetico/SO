/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
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
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
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
 * Funcion auxiliar que muestra los procesos de una lista con sus datos mas relevantes 
 */
void muestra_lista(lista_BCPs *lista){
	BCP *paux=lista->primero;
    /* Si hay un proceso mostramos que tipo de lista es mediante su estado */
    /* recorremos la lista mientras hayan procesos */
    while(paux){
        printk("\nProceso id %d {\n",paux->id);
        printk("\tEstado: ");

        if(paux){
            if (paux->estado == TERMINADO) printk("TERMINADO");
            else if (paux->estado == LISTO) printk("LISTO");
            else if (paux->estado == EJECUCION) printk("EJECUCION");
            else if (paux->estado == BLOQUEADO) printk ("BLOQUEADO");
            printk("\n");
        }

        printk("\tPrioridad: %d;\n",paux->prioridad);
        printk("\tPrioridad_E: %f;\n",paux->prioridad_efectiva);
        printk("}\n");
        paux = paux->siguiente;
    }
    return;
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

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/* 
 *Funcion que retorna el proceso con maxima prioridad
 */
static BCP * maxima_prioridad(lista_BCPs *lista){
    BCP * paux, * max_prio;
    paux = lista->primero;
    max_prio = paux;
    /* recorremos la lista y nos quedamos con el proceso con max_prio */
    while(paux->siguiente){
        paux = paux->siguiente;
        if(paux->prioridad > max_prio->prioridad) max_prio = paux;
    }
    return max_prio;
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
    /*  la plainificacion actual se base en la busqueda de maxima_prioridad */
	return maxima_prioridad(&lista_listos);
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
    int nivel;
    /* detenemos interrupciones */
	nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	//eliminar_primero(&lista_listos); /* proc. fuera de listos */
	
    eliminar_elem(&lista_listos, p_proc_actual); /* proc. fuera de listos */
    p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	
    /* Realizar cambio de contexto */
    //muestra_lista(&lista_listos);
	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
    p_proc_actual->estado=EJECUCION;
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    
    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;
    
    /*  volvemos a poner interrupciones como antes */
	fijar_nivel_int(nivel);
    
    return; /* no debería llegar aqui */
}

/*
 * Funcion auxiliar que pone un proceso en bloqueado
 * y hace un cambio de contexto
 *
 */
static void bloquear(lista_BCPs * lista){
    BCP * p_proc_anterior;
    int nivel;
   
    /* detenemos interrupciones */
	nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */

    /* bloqueamos el proceso y apuntamos a el */
    p_proc_actual->estado=BLOQUEADO;
    p_proc_anterior=p_proc_actual;


    /* lo elimino de lista listos y lo inserto en lista recibida*/
    eliminar_elem(&lista_listos, p_proc_anterior);
    insertar_ultimo(lista, p_proc_anterior);


    /* llamamos al planificador para recuperar el nuevo proceso */
    p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR BLOQUEO: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

    p_proc_actual->estado=EJECUCION;
    cambio_contexto(&(p_proc_anterior->contexto_regs), 
            &(p_proc_actual->contexto_regs));
    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;
    
    /*  volvemos a poner interrupciones como antes */
	fijar_nivel_int(nivel);

    return; /* no debería llegar aqui */
}


/*
 * Funcion auxiliar que pone desbloquea de la lista que recibeo 
 * y lo inserta en la lista listos
 *
 */
static void desbloquear(BCP * proc, lista_BCPs * lista){
    int nivel;
    
    /* lo marcamos como listo */
    proc->estado = LISTO;    
    nivel=fijar_nivel_int(NIVEL_3);
    eliminar_elem(lista, proc); /* lo eliminamos de la lista de bloqueados */
    insertar_ultimo(&lista_listos, proc); /* lo insertamos como listo */
    fijar_nivel_int(nivel);
    
    /* si no hay una replanificacion pendiente, comprobamos si es necesaria */
    if(!replanificacion_pendiente){
        /* comprobamos la prioridad del nuevo proceso */
        if(proc->prioridad > p_proc_actual->prioridad){
            /* activamos la interrupcion con la replanificacion pendiente */
            replanificacion_pendiente = 1;
            activar_int_SW();
        }
    }

    return;
}

/*
 * Funcion auxiliar que modifica el proceso actual por el que retorna
 * el planificador
 */
static void replanificar(){

    /* Mostramos lista listos */
    printk("-> PROCESOS LISTOS:\n");
    muestra_lista(&lista_listos);
    /* Mostramos lista dormidos */
    printk("-> PROCESOS BLOQUEADOS:\n");
    muestra_lista(&lista_dormidos);
    
    BCP * p_proc_anterior;
    /*  ponemos le proceso actual de EJECUCION a LISTO */
    p_proc_actual->estado=LISTO;
    p_proc_anterior = p_proc_actual;
    /* recuperamos el proceso segun el planificador */
    p_proc_actual = planificador();

	printk("-> C.CONTEXTO POR REPLANIFICACION: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);
    
    p_proc_actual->estado=EJECUCION;
    cambio_contexto(&(p_proc_anterior->contexto_regs), 
            &(p_proc_actual->contexto_regs));
    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;
    return;
}
/*
 * Funcion auxiliar que acutaliza los ticks de los procesos dormidos
 * cuando detecta que uno no tiene ticks pendientes lo envia a desbloquear
 *
 */

static void ajustar_dormidos(){
	BCP *paux=lista_dormidos.primero;
    BCP *paux_a_bloquear;

    while(paux){
       // printk("Proceso id: %i, ticks remaining: %i\n", paux->id, paux->nticks);
        paux->nticks--;
        if(paux->nticks == 0){
	        printk("-> DESPERTANDO PROC: %d\n", paux->id);
         
            // apuntamos al paux a bloquar 
            paux_a_bloquear = paux;
            //apuntamos al paux siguiente
            paux = paux->siguiente;
            //bloqueamos el paux a bloquar
            desbloquear(paux_a_bloquear, &lista_dormidos);
        }else{
            paux = paux->siguiente;
        }
    }
    return;
}
/*
 * funcion actual que reajusta las prioridades de todos los procesos si es necesario
 */
static void reajustar_prioridades(){
    int  contador, prioridad;
    float prioridad_e;
    printk("-> REAJUSTANDO TODOS LOS PROCESOS\n");
    for(contador = 0; contador < MAX_PROC; contador++){
        /* comprobamos que en el BCP hay un proceso */
        if(tabla_procs[contador].estado != NO_USADA){
            prioridad_e = tabla_procs[contador].prioridad_efectiva;
            prioridad = tabla_procs[contador].prioridad;
            tabla_procs[contador].prioridad_efectiva = (prioridad_e / 2.0) + prioridad;
        }
    }

}

/* 
 * Funcio que comprueba la necesidad de reajustar la prioridad de todos los procesos
 * con la condicion de que todos los listos tengan prioridad efectiva <= 0
 * */
static int comprobar_necesario_reajustar_prioridades(){
    BCP * paux;
    int nivel, reajustar_todos;

    reajustar_todos = 1; /* true por defecto */

    /* detenemos interrupciones */
    nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
    /* recorremos los procesos listos */
    paux = lista_listos.primero;
    while(paux){
        if( paux->prioridad_efectiva >= 0 ){
            reajustar_todos = 0;
            break;
        }
        paux = paux->siguiente;
    }
    /* activamos las interrupciones */
    fijar_nivel_int(nivel);

    return reajustar_todos;

}
/*
 * funcion auxiliar que actualiza la prioridad
 * del proceso actual
 */
static void ajustar_prioridad_actual(){

    //decrementamos la prio_efectiva del proceso actual
    p_proc_actual->prioridad_efectiva -=1.0;
    // si ha llegado a 0
    if(p_proc_actual <= 0){
        /* ahora podemos comprobar si es necesario reajustar prioridades */
        if(comprobar_necesario_reajustar_prioridades())
            reajustar_prioridades();
        /* ahora replanificamos */
        replanificacion_pendiente = 1;
        activar_int_SW();
    }
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

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
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
    // ajustamos prio del proceso actual
    ajustar_prioridad_actual();
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
    /* Si hay replanificacion pendiente */
    if(replanificacion_pendiente)
        replanificar();

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
    int nivel;
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
        p_proc->nticks = 0; /* inicializamos los ticks a 0*/
		p_proc->estado=LISTO;
        /* si hay proceso actual es el padre del nuevo */
        if (p_proc_actual){
            // la prioridad base se mantiene
            p_proc->prioridad = p_proc_actual->prioridad;
            /* comprobamos las condiciones para repartir la prio_efectiva */
            if(!(p_proc_actual->prioridad == MIN_PRIO &&
                    p_proc_actual->prioridad_efectiva <= MIN_PRIO)){
                /* dividimos la prioridad del padre
                 * por que se repartira con el hijo 
                 */
                p_proc_actual->prioridad_efectiva /=  2.0;
            }
            // asignamos la prioridad efectiva al hijo
            p_proc->prioridad_efectiva = p_proc_actual->prioridad_efectiva;
        }else{
            /* Si no existe proceso actual, es que es el proceso inicial
            * y fijamos su prio a min*/
            p_proc->prioridad = MIN_PRIO;
            //asignamos la efectiva asegurando que es un float
            p_proc->prioridad_efectiva = MIN_PRIO * 1.0;
        }

        /* detenemos interrupciones */
        nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
		
        /* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
    
        /*  volvemos a poner interrupciones como antes */
        fijar_nivel_int(nivel);
		
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

        return 0; /* no debería llegar aqui */
}

/* 
 * Tratamiento de llamada al sistema fijar_prio. Actualiza la prioridad del proceso
 * actual
 */
int sis_fijar_prio(){
    unsigned int prioridad, prioridad_anterior;
    float prioridad_efectiva_anterior;

//////////////////////////////////////////
    /* Mostramos lista listos */

    printk("-> PROCESOS LISTOS:\n");
    muestra_lista(&lista_listos);
    /* Mostramos lista dormidos */
    printk("-> PROCESOS BLOQUEADOS:\n");
    muestra_lista(&lista_dormidos);
////////////////////////////////////////////////

    prioridad=(unsigned int)leer_registro(1);

    /* comprobamos que sea una prioridad valida */
    if (prioridad < MIN_PRIO) return -1;
    if (prioridad > MAX_PRIO) return -1;
    
    printk("-> PROC %d, FIJANDO PRIORIDAD DE %d A %d\n",p_proc_actual->id, p_proc_actual->prioridad, prioridad);
    
    /* nos guardamos la prioridad actual como la anterior */
    prioridad_anterior = p_proc_actual->prioridad;
    prioridad_efectiva_anterior = p_proc_actual->prioridad_efectiva;
    p_proc_actual->prioridad = prioridad;
    /* comprobamos las condiciones para asignar la prio_efectiva */
    if(prioridad >= prioridad_anterior){
        /* la prio_e = prio_e_ant * (prio + prio_ant) /(2 prio_ant)*/
        printk("CAMBI A MAYOR\n");
        p_proc_actual->prioridad_efectiva *= ( (prioridad + prioridad_anterior)/(prioridad_anterior * 2.0) );
    }else{
        printk("CAMBI A MENOR\n");
        /* La prio_e = prio_e_ant * (prio / prio_ant) evitando problemas con enteros*/
        p_proc_actual->prioridad_efectiva *= (prioridad / (prioridad_anterior * (1.0)));
    }
    
    printk("-> PROC %d, FIJANDO PRIORIDAD_E DE %f A %f\n",p_proc_actual->id,
            prioridad_efectiva_anterior, p_proc_actual->prioridad_efectiva);

    /* comprobamos si se cumplen las condiciones para replanificar */
    if(p_proc_actual->prioridad_efectiva < prioridad_efectiva_anterior){
        /* si la prioridad_anterior es mayor, hay que replanificar */
        if(!replanificacion_pendiente){
            replanificacion_pendiente = 1;
            activar_int_SW();
        }
    
    }
    return 0;
    
} 

/*
 *
 * Rutina de inicialización invocada en arranque
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
