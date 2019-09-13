/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
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

	printk("-");

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
 * Función auxiliar que mete el proceso en ejecución en la lista destino
 * y realiza el cambio de contexto con el primero de la lista de listos
 *
 */
 
void cambio_proceso(lista_BCPs* lista_destino){
        BCPptr actual = p_proc_actual;
        
        int nivel=fijar_nivel_int(NIVEL_3);
        eliminar_elem(&lista_listos,actual);
        fijar_nivel_int(nivel);
        insertar_ultimo(lista_destino,actual);
       
        p_proc_actual = planificador();
        p_proc_actual->tiempo_proc = 10;
        cambio_contexto(&(actual->contexto_regs), &(p_proc_actual->contexto_regs));
}


//Funcion desbloquear mutex
int unlock_mutex_aux(unsigned int indice_descriptor){
    if(tabla_mutex[indice_descriptor].isCreated == 1){
        if(tabla_mutex[indice_descriptor].tipo == 0){ // Mutex no recursivo
           if(tabla_mutex[indice_descriptor].isBlocked == 1){
               if(p_proc_actual->id == tabla_mutex[indice_descriptor].id_proc){
                   if(tabla_mutex[indice_descriptor].num_procesos_bloqueados > 0){ //Existen procesos que quieren ese mutex
                       for(int i = 0; i< NUM_MUT_PROC; i++){
                           if(p_proc_actual->descriptores_mutex[i] == indice_descriptor){
                               p_proc_actual->descriptores_mutex[i] = -1;
                           }
                       }
                       tabla_mutex[indice_descriptor].id_proc = tabla_mutex[indice_descriptor].lista_espera.primero->id;
                       tabla_mutex[indice_descriptor].num_procesos_bloqueados -= 1;
                       
                       
                       int nivel=fijar_nivel_int(NIVEL_3);
                       insertar_ultimo(&lista_listos, tabla_mutex[indice_descriptor].lista_espera.primero);
                       eliminar_primero(&tabla_mutex[indice_descriptor].lista_espera);
                       fijar_nivel_int(nivel);
                       printk(" %s: Me he desbloqueado\n",tabla_mutex[indice_descriptor].nombre);
                       return 0;
                   }
                   else if(tabla_mutex[indice_descriptor].num_procesos_bloqueados == 0) {
                        for( int i = 0; i< NUM_MUT_PROC; i++){
                           if(p_proc_actual->descriptores_mutex[i] == indice_descriptor){
                               p_proc_actual->descriptores_mutex[i] = -1;
                           }
                        }
                        tabla_mutex[indice_descriptor].isBlocked = 0;
                        tabla_mutex[indice_descriptor].id_proc = -1;
                        printk(" %s: Me he desbloqueado\n",tabla_mutex[indice_descriptor].nombre);
                        return 0;
                    }
                }
               else{
                   printk("ERROR EN UNLOCK: Un proceso ha  intentado desbloquear un mutex que no le perteniecía \n");
                   return -1;
               }
           }

           else{
               printk("ERROR EN UNLOCK: El mutex no estaba bloqueado\n");
               return -1;
           }
    }   
        else if (tabla_mutex[indice_descriptor].tipo == 1){ //Mutex recursivo

            if(tabla_mutex[indice_descriptor].isBlocked == 1){
                if(p_proc_actual->id == tabla_mutex[indice_descriptor].id_proc){
                    if(tabla_mutex[indice_descriptor].num_bloqueos > 1){
                        tabla_mutex[indice_descriptor].num_bloqueos -= 1; 
                        return 0;
                    }
                    else if(tabla_mutex[indice_descriptor].num_bloqueos == 1){
                        
                        if(tabla_mutex[indice_descriptor].num_procesos_bloqueados > 0){ //existen procesos que quieren ese mutex
                            for(int i = 0; i< NUM_MUT_PROC; i++){
                                if(p_proc_actual->descriptores_mutex[i] == indice_descriptor){
                                    p_proc_actual->descriptores_mutex[i] = -1;
                                }
                            }
                            tabla_mutex[indice_descriptor].id_proc = tabla_mutex[indice_descriptor].lista_espera.primero->id;
                            tabla_mutex[indice_descriptor].num_procesos_bloqueados -= 1;
                            int nivel=fijar_nivel_int(NIVEL_3);
                            insertar_ultimo(&lista_listos, tabla_mutex[indice_descriptor].lista_espera.primero);
                            eliminar_primero(&tabla_mutex[indice_descriptor].lista_espera);
                            fijar_nivel_int(nivel);
                            printk(" %s: Me he desbloqueado\n",tabla_mutex[indice_descriptor].nombre);
                            return 0;
                    }
                        else if(tabla_mutex[indice_descriptor].num_procesos_bloqueados == 0) {
                             for( int i = 0; i< NUM_MUT_PROC; i++){
                                if(p_proc_actual->descriptores_mutex[i] == indice_descriptor){
                                    p_proc_actual->descriptores_mutex[i] = -1;
                                }
                             }
                             tabla_mutex[indice_descriptor].isBlocked = 0;
                             tabla_mutex[indice_descriptor].id_proc = -1;
                             printk(" %s: Me he desbloqueado\n",tabla_mutex[indice_descriptor].nombre);
                             return 0;
                         }    
                    }
                }
                else{
                   printk("ERROR EN UNLOCK: Un proceso ha  intentado desbloquear un mutex que no le perteniecía \n");
                   return -1;
                }  
            }
            else{
                printk("ERROR EN UNLOCK: El mutex no estaba bloqueado \n");
                return -1;
            }
           
        }
    } else {
        printk("ERROR EN LOCK: No existe ese mutex \n");
        return -1;
    }
    return -1;
    
}


//Función crear mutex 
int cerrar_mutex_aux( int mutexid){
    for(int i = 0; i<NUM_MUT_PROC; i++){
       if(p_proc_actual->descriptores_mutex[i] == mutexid){
           tabla_mutex[mutexid].proc_abiertos -= 1;
           p_proc_actual->descriptores_mutex[i]=-1;
           if(tabla_mutex[mutexid].id_proc == p_proc_actual->id){
               if(tabla_mutex[mutexid].tipo == 1 ){
                    if (tabla_mutex[mutexid].lista_espera.primero == NULL ){
                        tabla_mutex[mutexid].isBlocked = 0;
                        tabla_mutex[mutexid].num_bloqueos = 0;
                    } else {
                        int level = fijar_nivel_int(NIVEL_3);
                        BCPptr proc = tabla_mutex[mutexid].lista_espera.primero;
                        eliminar_primero(&tabla_mutex[mutexid].lista_espera);
                        insertar_ultimo(&lista_listos,proc);
                        fijar_nivel_int(level);
                        tabla_mutex[mutexid].num_bloqueos = 1;
                        tabla_mutex[mutexid].id_proc = proc->id;
                        tabla_mutex[mutexid].num_procesos_bloqueados-=1;
                    }
              
              }
              else {
                unlock_mutex_aux(mutexid);
               }
           }
           if(tabla_mutex[mutexid].proc_abiertos == 0){
               tabla_mutex[mutexid].isCreated = 0;
               if(lista_bloqueados_mutex_libre.primero != NULL){
                   BCPptr aux = lista_bloqueados_mutex_libre.primero;
                   int nivel=fijar_nivel_int(NIVEL_3);
                   eliminar_elem(&lista_bloqueados_mutex_libre, aux);
                   insertar_ultimo(&lista_listos, aux);
                   fijar_nivel_int(nivel);
               }
           }
           return 0;
       }  
   }
   return -1; 
}


/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
        for(int i = 0; i<NUM_MUT_PROC;i++){
            if (p_proc_actual->descriptores_mutex[i] >= 0){
                cerrar_mutex_aux(p_proc_actual->descriptores_mutex[i]);
            }
        }
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();
	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
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

//Funcion que escribe en el buffer lo que le llega de int_sw()
void escribir_buffer(char car){
     if(buffer.escribir == TAM_BUF_TERM){
        buffer.escribir = 0;
        buffer.buff[buffer.escribir] = car; 
        buffer.num_car ++;
        buffer.escribir ++;
    }
    else{
        buffer.buff[buffer.escribir] = car;
        buffer.escribir ++;
    }
}


/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);
   
        if(buffer.num_car != TAM_BUF_TERM){
            escribir_buffer(car);
        }
        
        if(buffer.num_car != 0){
            if(buffer.bloqueados_lectura.primero != NULL){
                BCPptr actual = buffer.bloqueados_lectura.primero;
                eliminar_elem(&buffer.bloqueados_lectura, actual);
                insertar_ultimo(&lista_listos, actual);
                
            }
        }
        return;
}
/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){
        int nivel = fijar_nivel_int(NIVEL_1);
	printk("-> TRATANDO INT. SW\n");
        if(lista_listos.primero == lista_listos.ultimo){
            p_proc_actual->tiempo_proc = 10;
        }
        else
        cambio_proceso(&lista_listos);
        fijar_nivel_int(nivel);
	return;
}


/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
        
	printk("/");
        BCPptr aux = lista_dormidos.primero;
        
        while (aux != NULL) {
          aux->segs -= 1;
          BCPptr aux2 = aux->siguiente;
          if(aux->segs == 0){
              int nivel=fijar_nivel_int(NIVEL_3);
              eliminar_elem(&(lista_dormidos), aux);
              insertar_ultimo(&(lista_listos), aux);
              fijar_nivel_int(nivel); 
          }
          aux=aux2;  
        }
        p_proc_actual->tiempo_proc -= 1;
        if(p_proc_actual->tiempo_proc <= 0){
            activar_int_SW();
        } 
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
 		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
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
                for(int i=0;i < NUM_MUT_PROC; i++){
                    p_proc->descriptores_mutex[i] = -1;
                }

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
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir(){
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

//Obtiene el id del proceso en ejecución.
int sis_obtener_id_pr(){
	return (p_proc_actual ->id);
}

/*
 * Duerme el proceso la cantidad de segs que se le pase
 * Cuando despierta pasa como ultimo a la lista de listos.
 */

int sis_dormir(){
    unsigned int  segs=(unsigned int)leer_registro(1);
    BCP *actual = p_proc_actual;
    actual->segs = segs*TICK;
    cambio_proceso(&lista_dormidos);
    return 0; 
}

/*
 * Función encargada de leer los caracteres escritos por el usuario
 * En la interrupción de terminal
 */

int leer_buffer(){
    if(buffer.leer == TAM_BUF_TERM){
        int caracter =  (int)buffer.buff[buffer.leer];
        buffer.leer = 0;
        buffer.num_car --;
        buffer.leer ++;
        return caracter;
    }
    else{
        int caracter =  (int)buffer.buff[buffer.leer];
        buffer.leer ++;
        return caracter;
    }
    return -1;
}

int sis_leer_caracter(){
    if(buffer.num_car == 0){
        cambio_proceso(&buffer.bloqueados_lectura);
    }
    else{
         int nivel = fijar_nivel_int(NIVEL_2);
         int caracter = leer_buffer();
         fijar_nivel_int(nivel);
         return caracter;
    }
    return -1;
}

/* Funciones auxiliares para mutex */

static void iniciar_tabla_mutex(){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		tabla_mutex[i].isBlocked = 0; /* indica que el mutex esta libre */
                tabla_mutex[i].isCreated = 0;
	}
}

static int buscar_descriptor_libre(){
	int i;
	for(i = 0; i < NUM_MUT_PROC; i++){
		if(p_proc_actual->descriptores_mutex[i] < 0)
			return i; /* devuelve el n˙mero del descriptor */
	}
	return -1; /* no hay descriptor libre */
}

static int buscar_nombre_mutex(char *nombre_mutex){
	int i;
	for(i = 0; i < NUM_MUT; i++){
            if(tabla_mutex[i].isCreated == 1){
                if(strcmp(tabla_mutex[i].nombre, nombre_mutex) == 0)
                            return i;	/* el nombre existe y devuelve su posicion en la tabla de mutex */
                }   
        }
	return -1; /* el nombre no existe */
}

static int buscar_mutex_libre(){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		if(tabla_mutex[i].isCreated == 0)
			return i;	/* el mutex esta libre y devuelve su posicion en la tabla de mutex */
	}
	return -1; /* no hay mutex libre */
}

/*
 * Llamada al sistema crear_mutex
 * parametro nombre en registro 1
 * parametro tipo en registro 2
 */
int sis_crear_mutex(){
	char * nombre = (char*)leer_registro(1);
	int tipo = (int)leer_registro(2);
	if(strlen(nombre) > MAX_NOM_MUT){
		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex demasiado largo\n");
		return -1;
	}
	if(buscar_nombre_mutex(nombre) >= 0){
		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex ya existente\n");
		return -2;
	}
	int descr_mutex_libre = buscar_descriptor_libre();
	if(descr_mutex_libre < 0){
		printk("(SIS_CREAR_MUTEX) Error: No hay descriptores de mutex libres\n");
		return -3;
	}else{
		printk("(SIS_CREAR_MUTEX) Devolviendo descriptor numero %d\n", descr_mutex_libre);
	}
	int pos_mutex_libre = buscar_mutex_libre();
	if(pos_mutex_libre < 0){
		printk("(SIS_CREAR_MUTEX) No hay mutex libre, bloqueando proceso\n");
		// bloquear a la espera de que quede alguno libre
		BCP * proc_a_bloquear=p_proc_actual;	
		proc_a_bloquear->estado=BLOQUEADO; 					
		int nivel_int = fijar_nivel_int(3);
		//printk("(SIS_CREAR_MUTEX) Eliminando proceso con PID %d de la cola de listos\n", proc_a_bloquear->id);
		eliminar_primero(&lista_listos); 
		//printk("(SIS_CREAR_MUTEX) Insertando proceso con PID %d en la lista de bloqueados mutex libre\n", proc_a_bloquear-> id);
		insertar_ultimo(&lista_bloqueados_mutex_libre, proc_a_bloquear); 			
		p_proc_actual=planificador();
		fijar_nivel_int(nivel_int);
		cambio_contexto(&(proc_a_bloquear->contexto_regs),
			 	&(p_proc_actual->contexto_regs));
	}else{
            
                strcpy(tabla_mutex[pos_mutex_libre].nombre,nombre); // se copia el nombre al mutex
                tabla_mutex[pos_mutex_libre].isCreated = 1;
		tabla_mutex[pos_mutex_libre].tipo = tipo; // se asigna el tipo
		tabla_mutex[pos_mutex_libre].isBlocked = 0; // se cambia el estado a OCUPADO
		tabla_mutex[pos_mutex_libre].num_bloqueos = 0;
		tabla_mutex[pos_mutex_libre].num_procesos_bloqueados = 0;
                tabla_mutex[pos_mutex_libre].proc_abiertos = 1;
                tabla_mutex[pos_mutex_libre].lista_espera.primero = NULL;
                tabla_mutex[pos_mutex_libre].lista_espera.ultimo = NULL;
		p_proc_actual->descriptores_mutex[descr_mutex_libre] = pos_mutex_libre; // el descr apunta al mutex libre en la tabla
	}			
	printk("He terminao\n");
        return descr_mutex_libre;
}

/* Busca una posicion libre en la tabla de descriptores de mutex del proceso*/
int buscarPosicion(BCPptr proceso){
    for(int i = 0; i<NUM_MUT_PROC; i++){
        if(proceso->descriptores_mutex[i] == -1){
            return i;
        }
    }
    return -1;  
}

/*
 * Funciones para manejar los mutex
 */

int sis_abrir_mutex(){
    char* nombre = (char*) leer_registro(1);
    int posicion = buscarPosicion(p_proc_actual);
    if(posicion != -1){
        for (int i= 0; i< NUM_MUT; i++){
            if(tabla_mutex[i].isCreated == 1){
                if(strcmp(tabla_mutex[i].nombre, nombre)==0){
                    p_proc_actual->descriptores_mutex[posicion]=i;
                    tabla_mutex[i].proc_abiertos += 1;
                    return i;
                }
            }

        }
    }
    return -1;
}

int sis_cerrar_mutex(){
   int mutexid = (int) leer_registro(1);
   return cerrar_mutex_aux(mutexid);
}

int sis_lock_mutex(){
    unsigned int indice_descriptor = (unsigned int)leer_registro(1);
   
    if(tabla_mutex[indice_descriptor].isCreated == 1){
        if(tabla_mutex[indice_descriptor].tipo == 0){
           if(tabla_mutex[indice_descriptor].isBlocked == 1){
               if(p_proc_actual->id == tabla_mutex[indice_descriptor].id_proc){
                    printk("ERROR EN LOCK MUTEX: el mutex  %s ya estaba bloqueado\n", tabla_mutex[indice_descriptor].nombre);
                    return -1;               
                }
               else{
                   tabla_mutex[indice_descriptor].num_procesos_bloqueados +=1;
                   cambio_proceso(&tabla_mutex[indice_descriptor].lista_espera);
               }
           }

           else{
               tabla_mutex[indice_descriptor].isBlocked = 1;
               tabla_mutex[indice_descriptor].num_bloqueos = 1;
               tabla_mutex[indice_descriptor].id_proc = p_proc_actual->id;
               printk("El mutex  %s ha sido bloqueado \n", tabla_mutex[indice_descriptor].nombre);
               return 0;
           }
    }   
        else if (tabla_mutex[indice_descriptor].tipo == 1){

            if(tabla_mutex[indice_descriptor].isBlocked == 1){
                if(p_proc_actual->id == tabla_mutex[indice_descriptor].id_proc){
                   tabla_mutex[indice_descriptor].num_bloqueos += 1; 
                   return 0;
                }
                else{
                tabla_mutex[indice_descriptor].num_procesos_bloqueados += 1;
                printk("El mutex  %s ha sido bloqueado\n ", tabla_mutex[indice_descriptor].nombre);
                cambio_proceso(&tabla_mutex[indice_descriptor].lista_espera);
                return 0;
                }
            }
            else{
                tabla_mutex[indice_descriptor].num_bloqueos+= 1;
                tabla_mutex[indice_descriptor].id_proc = p_proc_actual->id;
                tabla_mutex[indice_descriptor].isBlocked = 1;
                printk("El mutex  %s ha sido bloqueado \n", tabla_mutex[indice_descriptor].nombre);
                return 0;
        }

        }
    } else {
        printk("ERROR EN LOCK: No existe ese mutex \n");
        return -1;
    }
    return -1;
}

int sis_unlock_mutex(){
    unsigned int indice_descriptor = (unsigned int)leer_registro(1);
    return unlock_mutex_aux(indice_descriptor); 
}

int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
        iniciar_tabla_mutex();

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
        p_proc_actual->tiempo_proc = 10;
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
