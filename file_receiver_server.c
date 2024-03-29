#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFSIZ 1024
#define SOCKETERROR (-1)

typedef struct {
	char* data;
	int size;
}imagen;

FILE *fptr;

struct client {
    //Cantidad de clientes
    socklen_t client_len;
    struct sockaddr_in client_address;
    int client_sockfd;
    pthread_t thread;
    
};

enum { PORTSIZE = 5 };

 
void *forClient(void *ptr);
void menu();
int opciones_inicio();

//Limpiar los procesos que aun se queden escuchando en el puerto
void limpiador_puerto(const char* port_num) {
    char temp[100] = "sudo lsof -t -i tcp:";
    sprintf(temp, "%s%s%s", temp, port_num, " | xargs kill -9;");
    system(temp);
}

void sig_handler(int signo)
{
    if (signo == SIGINT)
        printf("Error de interrupcion \n");
}

int main(int argc, char **argv)
{

    puts("Server activo! \n");
    struct addrinfo hints, *res;

    //Sin problemas
    int disponible = 1;
    
    int server_sockfd;

    //Asignacion del puerto para conectarse
    unsigned short server_port = 9898u;
    char port_no[PORTSIZE];
    struct sockaddr_in server_address;
    struct protoent *protoent;
    //protocolo a utilizar "tcp"
    char tipo_puerto[] = "tcp";
 
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
    protoent = getprotobyname(tipo_puerto);
    if (protoent == NULL) {
        perror("Falla el protocolo tcp");
        exit(EXIT_FAILURE);
    }


    server_sockfd = socket(AF_INET,SOCK_STREAM,protoent->p_proto);

    if (server_sockfd == SOCKETERROR) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &disponible, sizeof(disponible)) < 0) {
        perror("setsockopt(SO_REUSEADDR) falla");
        exit(EXIT_FAILURE);
    }
    //Detalle para realizar la conexion
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port);
    
    //Chequear sin el bind, funciona correctamente
    if (bind(server_sockfd,(struct sockaddr*)&server_address,sizeof(server_address)) == SOCKETERROR) {
        perror(" Falla el bind");
        limpiador_puerto(argv[1]);
        exit(EXIT_FAILURE);
    }

    //Chequear sin el listen, funciona correctamente
    if (listen(server_sockfd, 5) == SOCKETERROR) {
        perror("Error en el listen");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Escuchando en el puerto: %d\n", server_port);

    //Creando los threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,1);

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
    pthread_attr_destroy(&attr);
    return EXIT_SUCCESS;
    menu();
    
    
}

int opciones_inicio () {
	puts("Opciones:");
	puts("	1. Ver Log // no terminado");
	puts("	2. Borrar Log // no terminado");
	puts("	3. Salir");
	puts("Introduzca su selección:");
	int inputs;
	scanf("%d",&inputs);
	return inputs;
}

void menu() {
	while (1) {
		int option = opciones_inicio();
		switch(option)
		{
			case 1:
                printf(">>>>>>No terminado aun \n");
				break;

			case 2:
				printf(">>>>>>>No terminado aun sorry, estamos trabajando \n");
				break;
			
			case 3:
                printf(">>>>>>>Esta si xd, bye! \n");
				exit(0);
				break;
			default:
				puts("Opcion Inválida!");
		}
	}

}


void * forClient(void *ptr)
{

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
    imagen bufferimg[300];
    
    char archivo[BUFSIZ];
    char cmd[BUFSIZ];
    int n=0;
    int xorVal;
    int i,j;


    // Número de subproceso significa la identificación del cliente
    // hasta dejar de recibir seguir tomando información
    while (recv(connect_socket, archivo, sizeof(archivo), 0)) {

        if((strcmp(archivo, "listServer") == 0
           || strcmp(archivo, "listLocal") == 0 || strcmp(archivo, "help") == 0
            || strcmp(archivo, "exit") == 0 || strcmp(archivo, "sendFile") == 0)) {
            printf("--- Command <%s> ---\n", archivo);
            continue;
        }

        file_path = archivo;
         
        //fprintf(stderr, "Archivo recibido:   =>  %s\n", file_path);
         
        filefd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (filefd == SOCKETERROR) {
            perror("falla el open");
            exit(EXIT_FAILURE);
        }
        do {
            FILE *fl = fopen(file_path,"wb");

            for(i = 0;i < n;i++){
                    j = 0;
                    while(j < bufferimg[i].size){
                        bufferimg[i].data[j] = bufferimg[i].data[j] ^ xorVal;
                        j++;
                    }
                    fwrite(bufferimg[i].data, 1, bufferimg[i].size, fl);
                    free(bufferimg[i].data);
                }
                fclose(fl);
                fprintf(stderr, "Archivo recibido:   =>  %s\n", file_path);

                //Close(connect_socket);
                //free(ctl);
            read_return = read(connect_socket, buffer, BUFSIZ);
            if (read_return == SOCKETERROR) {
                perror("falla el read");
                exit(EXIT_FAILURE);
            }
            if (write(filefd, buffer, read_return) == SOCKETERROR) {
                perror("falla el write");
                exit(EXIT_FAILURE);
            }
        } while (read_return > 0);
         
        close(filefd);
         
    }
     
    fprintf(stderr, "Se ha cerrado la conexion con el cliente\n");
    fprintf(stderr, "Servidor activo \n");
    menu();
    opciones_inicio();

    close(connect_socket);
    free(ctl);
    return (void *) 0;
    
}