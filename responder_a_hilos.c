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
#define MAX 256
#define MAX2 276
#define EQ(str1, str2) (strcmp((str1), (str2)) == 0)

int fifo_12, fifo_21;
char nombre_fifo_12[MAX], nombre_fifo_21[MAX];
char mensaje[MAX];
char str[MAX2];
char *logname, *getenv();
void fin_de_transmision(int sig)
{
    printf("FIN DE TRANSMISION\n");
    write(fifo_21, "Corto\n", 7);
    close(fifo_12);
    close(fifo_21);
    exit(0);
}
void *hilo_recepcion(void *arg)
{
    while (!EQ(mensaje, "Corto\n"))
    {
        fflush(stdout);
        read(fifo_12, mensaje, MAX2);
        printf("%s", mensaje);
    }
    exit(0);
}

void *hilo_envio(void *arg)
{
    //ESTE ENVIA
    while (!EQ(mensaje, "Corto\n"))
    {
        fgets(mensaje, sizeof(mensaje), stdin);
        strcpy(str, logname);
        strcat(str, ": ");
        strcat(str, mensaje);
        write(fifo_21, str, strlen(str) + 1);
    }
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Forma de uso: %s usuario\n", argv[0]); //LINEA DE ARGUMENTOS
        exit(-1);
    }
    logname = getenv("LOGNAME");
    if (EQ(logname, argv[1]))
    {
        fprintf(stderr, "Comunicacion con uno mismo no permitido\n");
        exit(0);
    }

    sprintf(nombre_fifo_12, "/tmp/%s_%s", argv[1], logname);
    sprintf(nombre_fifo_21, "/tmp/%s_%s", logname, argv[1]);

    //aqui parece que hay un ERROR u error de apertura de cualquiera de nombre_fifo

    if ((fifo_12 = open(nombre_fifo_12, O_RDONLY)) == -1 ||
        (fifo_21 = open(nombre_fifo_21, O_WRONLY)) == -1)
    {
        if (fifo_12 == -1)
            perror(nombre_fifo_12);
        else
            perror(nombre_fifo_21); //aqui
        exit(-1);
    }

    pthread_t h;
    pthread_t h1;

    pthread_create(&h, NULL, hilo_recepcion, NULL);
    pthread_create(&h1, NULL, hilo_envio, NULL);
    signal(SIGINT, fin_de_transmision);
    pthread_join(h, NULL);
    pthread_join(h1, NULL);
}
