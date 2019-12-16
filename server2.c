#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>                      /* getprotobyname */
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFSIZ 1024  
#define SOCKETERROR (-1)

struct client {
    socklen_t client_len;
    struct sockaddr_in client_address;
    int client_sockfd;
    pthread_t thread;
};

// NOTE: proporcionar suficiente espacio para un puerto de 4 dígitos + EOS char
enum { PORTSIZE = 5 };

double cpu_time_used;
clock_t start, end;

 
void *forClient(void *ptr);

void portCleaner(const char* port_num) {
    char temp[100] = "sudo lsof -t -i tcp:";
    sprintf(temp, "%s%s%s", temp, port_num, " | xargs kill -9;");
    system(temp);
    //puts(temp);
}

void sig_handler(int signo)
{
    if (signo == SIGINT)
        printf("!!  OUCH,  Error de interrupcion \n");
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *res;

    //Sin problemas
    int enable = 1;
    //int filefd;  // NOTE: this is never initialized/used
    int server_sockfd;

    //Asignacion del puerto para conectarse
    unsigned short server_port = 9898u;
    char portNum[PORTSIZE];
    struct sockaddr_in server_address;
    struct protoent *protoent;
    //protocolo a utilizar "tcp"
    char protoname[] = "tcp";
 
#if 0
    int socket_index = 0;
#else
    struct client *ctl;
#endif
     
    if (argc != 2) {
        fprintf(stderr, "Modo compilacion:   ./server  <puerto>\n");
        exit(EXIT_FAILURE);
    }
    server_port = strtol(argv[1], NULL, 10);
     
    /* Se crea el socket para escucharlo.. */
    protoent = getprotobyname(protoname);
    if (protoent == NULL) {
        perror("Falla el protocolo tcp");
        exit(EXIT_FAILURE);
    }


    server_sockfd = socket(AF_INET,SOCK_STREAM,protoent->p_proto);

    if (server_sockfd == SOCKETERROR) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port);
    
    if (bind(server_sockfd,(struct sockaddr*)&server_address,sizeof(server_address)) == SOCKETERROR) {
        perror(" Falla el bind");
        portCleaner(argv[1]);
        exit(EXIT_FAILURE);
    }
    if (listen(server_sockfd, 5) == SOCKETERROR) {
        perror("Error en el listen");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Escuchando en el puerto: %d\n", server_port);
 
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,1);
    start = clock();

    while (1) {

        ctl = malloc(sizeof(struct client));
        if (ctl == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        
        //Dinamicamente se identifica cuantos clientes van a establecer la conexion con el servidor
        ctl->client_len = sizeof(ctl->client_address);
        puts("Esperando a los clientes\n");
         
        ctl->client_sockfd = accept(server_sockfd,(struct sockaddr *) &ctl->client_address, &ctl->client_len);
        if (ctl->client_sockfd < 0) {
            perror("No se puede aceptar la connexion\n");
            close(server_sockfd);
            exit(EXIT_FAILURE);
        }
        
        /* Creaando los threads segun la cantidad de peticiones del cliente*/
        pthread_create(&ctl->thread, &attr, forClient, ctl);
    }
     
    return EXIT_SUCCESS;
}


void * forClient(void *ptr)
{
    end = clock();
    cpu_time_used = 1000 * (((double) (end - start)) / CLOCKS_PER_SEC);
#if 0
    int connect_socket = (int) ptr;
#else
    struct client *ctl = ptr;
    int connect_socket = ctl->client_sockfd;
#endif
    int filefd;
    ssize_t read_return;
    char buffer[BUFSIZ];
    char *file_path;
    char archivo[BUFSIZ];
    char cmd[BUFSIZ];


    // Número de subproceso significa la identificación del cliente
    //printf("Conectado en el tiempo:  [%lf] ---  Thread number [%ld]\n", cpu_time_used, pthread_self());
 
    // hasta dejar de recibir seguir tomando información
    while (recv(connect_socket, archivo, sizeof(archivo), 0)) {

        if((strcmp(archivo, "listServer") == 0
           || strcmp(archivo, "listLocal") == 0 || strcmp(archivo, "help") == 0
            || strcmp(archivo, "exit") == 0 || strcmp(archivo, "sendFile") == 0)) {
            printf("--- Command <%s> ---\n", archivo);
            continue;
        }

        file_path = archivo;
         
        fprintf(stderr, "Archivo recibido:   =>  %s\n", file_path);
         
        filefd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (filefd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        do {
            read_return = read(connect_socket, buffer, BUFSIZ);
            if (read_return == SOCKETERROR) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            if (write(filefd, buffer, read_return) == SOCKETERROR) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        } while (read_return > 0);
         
        // NOTE/BUG: filefd was never closed
        close(filefd);
         
    }
     
    fprintf(stderr, "Se ha cerrado la conexion con el cliente\n");
     
    // NOTE: do all client related cleanup here
    // previously, the main thread was doing the close, which is why it had
    // to do the pthread_join
    close(connect_socket);
    free(ctl);
     
    return (void *) 0;
}