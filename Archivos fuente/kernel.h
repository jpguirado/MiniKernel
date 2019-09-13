/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H



#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include <stdio.h>
#include <string.h>




/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */

typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
        
        /*NUEVO*/
       int descriptores_mutex[NUM_MUT_PROC]; //Por cada mutex activo se almacenará su posición en la tabla global de mutex.
       unsigned int segs;
       int tiempo_proc;
        
} BCP;



/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


typedef struct buffer_terminal_t{
    char buff [TAM_BUF_TERM];
    int leer;
    int escribir;
    int num_car;
    lista_BCPs bloqueados_lectura;
} Buffer_terminal;


typedef struct Mutex_t *Mutexptr;

typedef struct Mutex_t  {
    int tipo;
    char nombre[MAX_NOM_MUT];
    int isBlocked;
    int num_bloqueos;
    int num_procesos_bloqueados;
    lista_BCPs lista_espera;
    int id_proc;
    int isCreated;
    int proc_abiertos;
} Mutex;

/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];


Mutex tabla_mutex[NUM_MUT];

Buffer_terminal buffer;
/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

lista_BCPs lista_dormidos ={NULL,NULL};

lista_BCPs lista_bloqueados_mutex_libre = {NULL,NULL};

/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int sis_obtener_id_pr();
int sis_dormir();
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock_mutex();
int sis_unlock_mutex();
int sis_cerrar_mutex();
int sis_leer_caracter();


/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{sis_obtener_id_pr},
                                        {sis_dormir},
                                        {sis_crear_mutex},
                                        {sis_abrir_mutex},
                                        {sis_lock_mutex},
                                        {sis_unlock_mutex},
                                        {sis_cerrar_mutex},
                                        {sis_leer_caracter}};

#endif /* _KERNEL_H */

