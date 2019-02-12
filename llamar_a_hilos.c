//CODIGO ORIGINAL DEL AUTOR  FRANCISCO MANUEL MARQUEZ GARCÍA
//MODIFICADO POR ACOSTA LARA ESAU

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "utmp.h"
#define MAX 256
#define MAX2 276

#define EQ(str1, str2) (strcmp((str1), (str2)) == 0) //comparar dos cadenas

int fifo_12, fifo_21;
char nombre_fifo_12[MAX], nombre_fifo_21[MAX];
char mensaje[MAX];
char str[MAX2];
int tty;
char terminal[MAX], *logname, *getenv();
struct utmp *utmp, *getutent(); //utmp es un archivo sobre
//quien esta logueado en el sistema,
//getutent lee la linea en la que se encuentra del archivo utmp
//la anterior linea significa crear una variable local con la estructura de utmp.
//y otra *getutent.
int com_closed;
void fin_de_transmision(int sig)
{
    printf("FIN DE TRANSMISION\n");
    write(fifo_12, "Corto\n", 7);
    close(fifo_12);
    close(fifo_21);

    exit(0);
}

void *hilo_recepcion(void *arg)
{
    while (!EQ(mensaje, "Corto\n"))
    {
        fflush(stdout);
        read(fifo_21, mensaje, MAX2);
        printf("%s", mensaje);
    }
    exit(0);
}

void *hilo_envio(void *arg)
{
    while (!EQ(mensaje, "Corto\n"))
    {
        fgets(mensaje, sizeof(mensaje), stdin);
        strcpy(str, logname);
        strcat(str, ": ");
        strcat(str, mensaje);
        write(fifo_12, str, strlen(str) + 1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) //si lo que entra es diferente de 2, osea si no hay nada en el argumento
    {
        fprintf(stderr, "Forma de uso: %s usuario\n", argv[0]);
        exit(-1); //con esto se finaliza el programa
    }

    logname = getenv("LOGNAME"); //devuelve un puntero
    //a una cadena que contiene el valor para el nombre especificado.
    //Si el nombre especificado
    //no se puede encontrar en el entorno del proceso de llamada,
    //se devolverá un puntero nulo.
    if (EQ(logname, argv[1])) //el logname es igual al mismo usuario, no se debe
    {
        fprintf(stderr, "Comunicacion con uno mismo no permitido\n"); //mensaje de error
        exit(0);
    }

    while ((utmp = getutent()) != NULL &&
           strncmp(utmp->ut_user, argv[1], sizeof(utmp->ut_user)) != 0)
        ; /*el valor de la estructura utmp se compara con lo ingresado con el tamaño de ut_user  y utmp es igual a getutent, el ciclo se mantiene mientras utmp no sea nulo y la comparacion no sea 0, osea falso*/

    if (utmp == NULL) //si utmp es nulo, significa que el usuario no ha iniciado sesion
    {
        printf("EL USUARIO %s NO HA INICIADO SESION\n", argv[1]);
        exit(0);
    }
    sprintf(nombre_fifo_12, "/tmp/%s_%s", logname, argv[1]);
    sprintf(nombre_fifo_21, "/tmp/%s_%s", argv[1], logname);
    /*en nombre fifo se guarda /tmp/esau_prueba1 lo cual es una carpeta que se genera*/

    unlink(nombre_fifo_12); //borra el archivo con ese nombre
    unlink(nombre_fifo_21);
    umask(~0666); //permisos en modo 0666????

    if (mkfifo(nombre_fifo_12, 0666) == -1) /*mkfifo regresa 0 si estuvo bien -1 si fallo */
    {
        perror(nombre_fifo_12); //error de permiso denegado
        exit(-1);
    }

    if (mkfifo(nombre_fifo_21, 0666) == -1)
    {
        perror(nombre_fifo_21);
        exit(-1);
    }
    sprintf(terminal, "/dev/%s", utmp->ut_line); /*escribe en el buffer de la terminal la cadena /dev/tty .... tal*/
    char command[50];
    strcpy(command, "sudo chmod 777 ");
    char command2[50];
    strcpy(command2, strcat(command, terminal));
    while ((tty = open(terminal, O_WRONLY)) == -1) //si no abre, manda error de permisos
    {
        printf("Ejecuta otra vez el mismo comando\n");
        perror(terminal);
        system(command2);
    }

    sprintf(mensaje, "\n\t\tLLAMADA PROCEDENTE DEL USUARIO %s\07\07\07\n"
                     "\t\tRESPONDER ESCRIBIENDO: responder-a %s\n\n",
            logname, logname); /*escribe la cadena en la variable mensaje*/

    write(tty, mensaje, strlen(mensaje) + 1); /*comunicacion a otro usuario*/
    close(tty);

    printf("Esperando respuesta...\n");
    if ((fifo_12 = open(nombre_fifo_12, O_WRONLY)) == -1 ||
        (fifo_21 = open(nombre_fifo_21, O_RDONLY)) == -1)
    {
        if (fifo_12 == -1)
            perror(nombre_fifo_12);
        else
            perror(nombre_fifo_21);
        exit(-1);

        /*Si hubo errores en la apertura de los archivos de "comunicacion"*/
    }
    printf("LLAMADA ATENDIDA\07\07\07\n");
    pthread_t h;
    pthread_t h1;

    //???
    /*Establecida toda la comunicacion, se pasa a hablar entre los usuarios*/
    pthread_create(&h, NULL, hilo_envio, NULL);
    pthread_create(&h1, NULL, hilo_recepcion, NULL);
    signal(SIGINT, fin_de_transmision);
    pthread_join(h, NULL);
    pthread_join(h1, NULL);
}
