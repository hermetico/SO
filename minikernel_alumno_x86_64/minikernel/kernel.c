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
 * Inserta un BCP de forma ordenada en la lista
 * Ordena segun prioridad_efectiva de forma descendente
 * Si hay procesos con la misma prioridad, se inserta el ultimo de ellos
 */
static void insertar_ordenado(lista_BCPs *lista, BCP *proc){
    /*  comprobamos que la lista no este vacia */
    if(lista->primero==NULL){
        lista->primero = proc;
        lista->ultimo = proc;
        proc->siguiente = NULL;
    }else if(lista->primero->prioridad_efectiva < proc->prioridad_efectiva ){
        /*  el nuevo proceso ha de ir el primero */
        proc->siguiente = lista->primero;
        lista->primero = proc;
    }else if(lista->ultimo->prioridad_efectiva >= proc->prioridad_efectiva ){
        /* el nuevo proceso ha de ir el ultimo */
        lista->ultimo->siguiente = proc;
        lista->ultimo = proc;
        proc->siguiente = NULL;
    }else{ 
        /* buscamos su posicion */
        BCP *paux=lista->primero; //puntero auxiliar
        while(paux->siguiente->prioridad_efectiva >= proc->prioridad_efectiva)paux = paux->siguiente;
        /* en este punto paux tiene prio_e mayor o igual a proc y paux->siguiente tiene prio_e menor */
        proc->siguiente = paux->siguiente;
        paux->siguiente = proc;
    }
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
 * Extrae el proceso de la lista y lo vuelve a insertar
 * de forma ordenada
 */
static void reordenar_elem(lista_BCPs *lista, BCP *proc){
    int nivel;
    nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
    eliminar_elem(lista, proc); //eliminamos el elemento de la lista
    insertar_ordenado(lista, proc); // lo insertamos ordenado
    fijar_nivel_int(nivel);
}

/*
 * Funcion auxiliar que muestra los procesos de una lista con sus datos mas relevantes 
 */
static void muestra_lista(lista_BCPs *lista){
	BCP *paux=lista->primero;
    int cierre_lista = 0;
    /* Si hay un proceso mostramos que tipo de lista es mediante su estado */
    /* recorremos la lista mientras hayan procesos */
    if(paux){
        printk("\n== LISTA DE PROCESOS ");
        cierre_lista = 1;
        if (paux->estado == LISTO || paux->estado == EJECUCION) printk("LISTOS");
        else if (paux->estado == BLOQUEADO) printk ("BLOQUEADOS");
        printk("\n");
    }
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
        printk("\tId padre %d;\n",paux->id_padre);
        printk("\tPrioridad: %d;\n",paux->prioridad);
        printk("\tPrioridad_E: %d;\n",paux->prioridad_efectiva);
        printk("\tnTicks: %d;\n", paux->nticks);
        printk("\tnHijos: %d:\n", paux->nfills);
        printk("}\n");
        paux = paux->siguiente;
    }
    if(cierre_lista)
        printk("== FIN LISTA\n\n");
    return;
}


/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */



/* 
 * Funcion que aplica una prioridad_efectiva a un proceso y lo reordena
 */
static void aplica_prioridad_efectiva(lista_BCPs *lista, BCP *proc, int prioridad){
    /* comprobamos que la prioridad no sigue siendo la misma */
    if(proc->prioridad_efectiva != prioridad){
        proc->prioridad_efectiva = prioridad; //aplicamos la prioridad
        reordenar_elem(lista, proc); // lo reordenamos
    }
}

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
/* *
 *Funcion que asigna a los hijos del proceso actual la id_padre ID_HUERFANO
 * */
static void tratar_hijos(){
    int  contador;
    printk("-> TRATANDO HIJOS DEL PROCESO %i\n", p_proc_actual->id);
    for(contador = 0; contador < MAX_PROC; contador++){
        /* comprobamos que en el BCP hay un proceso */
        if(tabla_procs[contador].estado != NO_USADA){
            /*  comprobamos que es hijo del proceso actual */
            if(tabla_procs[contador].id_padre == p_proc_actual->id)
                tabla_procs[contador].id_padre = ID_INIT;
        }
    }
}

/*
 * funcion actual que reajusta las prioridades de todos los procesos si es necesario
 */
static void reajustar_prioridades(){
    int  contador, prioridad;
    int prioridad_e, nueva_prio_e;;
    printk("NECESARIO REAJUSTE GLOBAL DE PRIORIDADES\n");
    for(contador = 0; contador < MAX_PROC; contador++){
        /* comprobamos que en el BCP hay un proceso */
        if(tabla_procs[contador].estado != NO_USADA){

            prioridad_e = tabla_procs[contador].prioridad_efectiva;
            prioridad = tabla_procs[contador].prioridad;
            nueva_prio_e = ( prioridad_e/2 ) + prioridad;

            if(tabla_procs[contador].estado == LISTO ||
                    tabla_procs[contador].estado == EJECUCION){
                //si es un proceso que esta en lista listos, debemos mantenerla ordenada
                aplica_prioridad_efectiva(&lista_listos, &tabla_procs[contador], nueva_prio_e);
            }else{
                tabla_procs[contador].prioridad_efectiva = (prioridad_e / 2) + prioridad;
            }
        }
    }

}

//No necesario
///* 
// *Funcion que retorna el proceso con maxima prioridad
// */
//static BCP * maxima_prioridad(lista_BCPs *lista){
//    BCP * paux, * max_prio;
//    paux = lista->primero;
//    max_prio = paux;
//    /* recorremos la lista y nos quedamos con el proceso con max_prio */
//    while(paux->siguiente){
//        paux = paux->siguiente;
//        if(paux->prioridad_efectiva > max_prio->prioridad_efectiva) max_prio = paux;
//    }
//    /* Esta parte de aqui pasa  decidirla el planificador */
////    /* si la maxima prioridad del proceso es 0, es necesario reajuste */
//    if(max_prio->prioridad_efectiva == 0){
//        reajustar_prioridades();
//        return maxima_prioridad(lista);
//    }
//
//    return max_prio;
//}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
    //BCP *max_prio;
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
    /* Lista listos es una lista ordenada 
     * Si la prioridad_efectiva del primero es 0,, reajustamos prioridades
     */
    if(lista_listos.primero->prioridad_efectiva == 0) reajustar_prioridades();
    /* devolvemos el primero */
    return lista_listos.primero;
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

    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;
    
    p_proc_actual->estado=EJECUCION;

    /* Mostramos lista listos */
    muestra_lista(&lista_listos);
    /* Mostramos lista dormidos */
    muestra_lista(&lista_dormidos);
    /*  realizamos el cambio de contexto */
    cambio_contexto(&(p_proc_anterior->contexto_regs), 
            &(p_proc_actual->contexto_regs));
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
    insertar_ordenado(&lista_listos, proc); /* lo insertamos como listo */
    fijar_nivel_int(nivel);
    
    /* si no hay una replanificacion pendiente, comprobamos si es necesaria */
    if(!replanificacion_pendiente){
        /* comprobamos la prioridad del nuevo proceso */
        if(proc->prioridad_efectiva > p_proc_actual->prioridad_efectiva){
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
    BCP * p_proc_anterior, *p_proc_nuevo;
    int nivel;
    /* detenemos interrupciones */
	nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
    
    /* comprobamos que el planificador no nos retorna el mismo proceso
     * que el actual, si es asi no es necesario replanificar*/
    p_proc_nuevo = planificador();
    if(p_proc_nuevo == p_proc_actual) return;

    /*  ponemos le proceso actual de EJECUCION a LISTO */
    p_proc_actual->estado=LISTO;
    p_proc_anterior = p_proc_actual;
    /* recuperamos el proceso segun el planificador */
    p_proc_actual = p_proc_nuevo;

    
    printk("-> C.CONTEXTO POR REPLANIFICACION: de %d a %d\n",
        p_proc_anterior->id, p_proc_actual->id);
    
    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;

    p_proc_actual->estado=EJECUCION;
    
    /* Mostramos lista listos */
    muestra_lista(&lista_listos);
    /* Mostramos lista dormidos */
    muestra_lista(&lista_dormidos);
	fijar_nivel_int(nivel);
    /* realizamos el cambio de contexto */
    cambio_contexto(&(p_proc_anterior->contexto_regs), 
            &(p_proc_actual->contexto_regs));
    return /* no deberia llegar aqui */;
}
/**
 * Funcion que resta al padre el hijo que va a ser eliminado y lo 
 * desbloquea en caso de que no tenga hijos pendientes
 * 
 */
static void tratar_padre(){
    BCP *padre;
    padre = &tabla_procs[p_proc_actual->id_padre];
    printk("-> TRATANDO PADRE (%i) DEL PROCESO %i\n",padre->id,
            p_proc_actual->id);

    //inicialmente solo restamos 1 al numero de hijos
    padre->nfills -= 1;
    // si no le quedan hijos pendientes
    if(!padre->nfills && padre->estado == BLOQUEADO){
        //aumentamos su prio efectiva
        padre->prioridad_efectiva = ((float) padre->prioridad_efectiva )* 1.1;
        desbloquear(padre, &lista_espera);
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

    /* modificamos la id_padre de los hijos a huerfano */
    if(p_proc_actual->nfills)
        tratar_hijos();
    
    /* tratamos al padre */
    if(p_proc_actual->id_padre != ID_HUERFANO)
        tratar_padre();

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

    /* Cancelamos la replanificacion que pueda haber pendiente */
    replanificacion_pendiente = 0;
	liberar_pila(p_proc_anterior->pila);
    p_proc_actual->estado=EJECUCION;
    /*  realizamos el cambio de contexto */
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    /*  volvemos a poner interrupciones como antes */
	fijar_nivel_int(nivel);
    
    return; /* no debería llegar aqui */
}

///* 
// * Funcio que comprueba la necesidad de reajustar la prioridad de todos los procesos
// * con la condicion de que todos los listos tengan prioridad efectiva <= 0
// * */
//static int comprobar_necesario_reajustar_prioridades(){
//    BCP * paux;
//    int nivel, reajustar_todos;
//
//    printk("-> COMPROBACION DE REAJUSTE GLOBAL DE PROCESOS,");
//    reajustar_todos = 1; /* true por defecto */
//
//    /* detenemos interrupciones */
//    nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
//    /* recorremos los procesos listos */
//    paux = lista_listos.primero;
//    while(paux){
//        if( paux->prioridad_efectiva > 0 ){
//            printk("RESULTADO : REAJUSTE NO NECESARIO\n");
//            reajustar_todos = 0;
//            break;
//        }
//        paux = paux->siguiente;
//    }
//    /* activamos las interrupciones */
//    fijar_nivel_int(nivel);
//
//    return reajustar_todos;
//
//}


/*
 * funcion auxiliar que actualiza la prioridad
 * del proceso actual
 */
static void ajustar_prioridad_actual(){

    /* Hay ocasiones que el proceso actual esta bloqueado y no hay ninguno listo */
    if(p_proc_actual->estado != EJECUCION ) return;
    //decrementamos la prio_efectiva del proceso actual
    aplica_prioridad_efectiva(&lista_listos, p_proc_actual, p_proc_actual->prioridad_efectiva - 1);
    // si ha llegado a 0
    if(p_proc_actual->prioridad_efectiva == 0){
        printk("-> PROCESO %d AGOTA TIEMPO DE USO DE CPU\n",p_proc_actual->id);
        /* ahora replanificamos siempre que el planificador nos de un proceso diferente*/
        if(p_proc_actual != planificador()){
            replanificacion_pendiente = 1;
            activar_int_SW();
        }
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
        p_proc->nticks = 0; /* inicializamos los ticks a 0 */
        p_proc->nfills = 0; /* inicializamos los hijos a 0 */
		p_proc->estado=LISTO;
        /* si hay proceso actual es el padre del nuevo */
        if (p_proc_actual){
            //aumentamos en 1 el numero de hijos
            p_proc_actual->nfills += 1;
            // incluimos la id del padre
            p_proc->id_padre = p_proc_actual->id;
            // la prioridad base se mantiene
            p_proc->prioridad = p_proc_actual->prioridad;
            /* comprobamos las condiciones para repartir la prio_efectiva */
            if(!(p_proc_actual->prioridad == MIN_PRIO &&
                    p_proc_actual->prioridad_efectiva <= MIN_PRIO)){
                /* dividimos la prioridad del padre
                 * por que se repartira con el hijo 
                 */
                //p_proc_actual->prioridad_efectiva /=  2;
                aplica_prioridad_efectiva(&lista_listos, p_proc_actual, p_proc_actual->prioridad_efectiva / 2.0);
            }
            // asignamos la prioridad efectiva al hijo
            p_proc->prioridad_efectiva = p_proc_actual->prioridad_efectiva;

            /*  comprobamos que el padre sigue siendo el mas prioritario */
            if(p_proc_actual != planificador()){
                replanificacion_pendiente = 1;
                activar_int_SW();
            }
        }else{
            //incluimod la id de huerfano
            p_proc->id_padre = ID_HUERFANO;
            /* Si no existe proceso actual, es que es el proceso inicial
            * y fijamos su prio a min*/
            p_proc->prioridad = MIN_PRIO;
            //asignamos la efectiva
            p_proc->prioridad_efectiva = MIN_PRIO;
        }

        /* detenemos interrupciones */
        nivel=fijar_nivel_int(NIVEL_3); /*nivel 3 detiene todas */
		
        /* lo insertamos ordenado en lista listos */
		insertar_ordenado(&lista_listos, p_proc);
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
 * Tratamiento de llamada al sistema get_ppid. Muestra el id del 
 * padre del proceso actual
 */
int sis_get_ppid(){
    printk("-> A DINS DE SIS_GET_PPID\n");
    return p_proc_actual->id_padre;

}

/*
 * Tratamiento de llamada al sistema dormir. Pone el proceso actual 
 * a dormir y pasa al siguiente proceso
 */
int sis_dormir(){

    unsigned int segundos;
    segundos=(unsigned int)leer_registro(1);
    // comprobamos que sean segundos  > que 0
    if (segundos <= 0) return 0;


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
    int prioridad_efectiva_anterior, prioridad_efectiva_resultante;

    /* Obtenemos la prioridad a aplicar */
    prioridad=(unsigned int)leer_registro(1);

    /* comprobamos que sea una prioridad valida */
    if (prioridad < MIN_PRIO) return -1;
    if (prioridad > MAX_PRIO) return -1;
    
    printk("-> PROC %d, FIJANDO PRIORIDAD DE %d A %d\n",p_proc_actual->id, p_proc_actual->prioridad, prioridad);
   
    //printk("Efectiva %i\n",p_proc_actual->prioridad_efectiva);

    /* nos guardamos la prioridad actual (base y efectiva) como la anterior */
    prioridad_anterior = p_proc_actual->prioridad;
    prioridad_efectiva_anterior = p_proc_actual->prioridad_efectiva;
    prioridad_efectiva_resultante = p_proc_actual->prioridad_efectiva;
    p_proc_actual->prioridad = prioridad; /*  asignamos la prioridad base */
    /* comprobamos las condiciones para asignar una prio_efectiva u otra */
    if(prioridad >= prioridad_anterior){
        //printk("Prioridad aumenta\n");
        /* la prio_e = prio_e_ant * (prio + prio_ant) /(2 prio_ant)*/
        prioridad_efectiva_resultante *= ( (prioridad + prioridad_anterior)/(prioridad_anterior * 2.0) );
    }else{
        //printk("Prioridad disminuye\n");
        /* La prio_e = prio_e_ant * (prio / prio_ant) evitando problemas con enteros*/
        prioridad_efectiva_resultante *= (prioridad / (prioridad_anterior * 1.0 ));
    }

    //printk("Efectiva resultante %i\n",prioridad_efectiva_resultante);

    /*  aplicamos la nueva prioridad efectiva */
    aplica_prioridad_efectiva(&lista_listos, p_proc_actual, prioridad_efectiva_resultante);
    printk("-> PROC %d, FIJANDO PRIORIDAD_E DE %d A %d\n",p_proc_actual->id, 
            prioridad_efectiva_anterior, p_proc_actual->prioridad_efectiva);

    //printk("Efectiva aplicada %i\n",p_proc_actual->prioridad_efectiva);

    /* Mostramos lista listos */
    muestra_lista(&lista_listos);
    /* Mostramos lista dormidos */
    muestra_lista(&lista_dormidos);

    /* comprobamos si se cumplen las condiciones para replanificar */
    if(p_proc_actual->prioridad_efectiva < prioridad_efectiva_anterior){
        /* comprobamos que no sigue siendo el mismo con la max prio */
        if(p_proc_actual != planificador()){
            /* si la prioridad_anterior es mayor, hay que replanificar */
            if(!replanificacion_pendiente){ /* comprobamos que no haya ya una replanificacino pendiente */
                replanificacion_pendiente = 1;
                activar_int_SW();
            }
        }
    }
    return 0;
}

/**
 *
 *
 */

int sis_espera(){
    /* si no tiene hijos */
    if(!p_proc_actual->nfills) return -1;
    printk("-> PROC %i ESPERA A SUS HIJOS\n", p_proc_actual->id);
    bloquear(&lista_espera);/* mandamos al proc a lista_espera */
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
