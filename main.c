#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "SerialManager.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define NBYTESINCOMINGSERIAL 16
#define TIMESLEEPREAD 0.01
#define TIMESLEEPTCP 0.01
#define NPUERTOSERIAL 1
#define BAUDRATE 115200
#define BUFFERMAXSIZE 128
#define UARTBPULSADOR 14


char datoUpLoad;//variable global que comparten los 2 hilos usada para pasar el pulsador presionado

//pthread_mutex_t mutexPulsador=PTHREAD_MUTEX_INITIALIZER;//protege la variable compartida datoUpLoad
static pthread_t SerialHandle, TcpHandel; //hilos

void bloquearSign(void);
void desbloquearSign(void);
int newfd;

void manejoInt(int sig)
{
	
	if((pthread_cancel(SerialHandle))!=0)
	{
		perror("Error al cerrar el hilo serial");	
	}
	if((pthread_cancel(TcpHandel))!=0)
	{
		perror("Error al cerrar el hilo tcp");
	}
}
void manejoTerm(int sig)
{
	if((pthread_cancel(SerialHandle))!=0)
	{
		perror("Error al cerrar el hilo serial");	
	}
	if((pthread_cancel(TcpHandel))!=0)
	{
		perror("Error al cerrar el hilo tcp");
	}
}



void* Tcp_handle (void* message)
{
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[BUFFERMAXSIZE];
	//char bufferUpLoad[BUFFERMAXSIZE];
	char bufferDownLoad[BUFFERMAXSIZE];
	//int newfd;
	int n;
	int s;

	// Creamos socket
	 if((s = socket(AF_INET,SOCK_STREAM, 0))==-1)
	 {
		 perror("Creación de socket");
	 }

	// Cargamos datos de IP:PORT del server
    	bzero((char*) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET;
    	serveraddr.sin_port = htons(10000);

    	if(inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr))<=0)
    	{
        	fprintf(stderr,"ERROR invalid server IP\r\n");
        	//return 1;
    	}

	// Abrimos puerto con bind()
	if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
		close(s);
		perror("listener: bind");
		//return 1;
	}

	// Seteamos socket en modo Listening
	if (listen (s, 10) == -1) // backlog=10
  	{
    	    	perror("error en listen");
    		exit(1);
  	}

	while(1)
	{
		// Ejecutamos accept() para recibir conexiones entrantes
		
		addr_len = sizeof(struct sockaddr_in);
    		if ( (newfd = accept(s, (struct sockaddr *)&clientaddr, 
                                        &addr_len)) == -1)
      		{
		      perror("error en accept");
		      exit(1);
	    	}
	 	
		char ipClient[32];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf  ("server:  conexion desde:  %s\n",ipClient);

		while(1)
		{
			if( (n = read(newfd,buffer,BUFFERMAXSIZE))<=0 )
			{
				perror("Error leyendo mensaje en socket");
				//exit(1);
				break;
			}
			else
			{
				if(buffer[0]==':')
				{
										
					sprintf(bufferDownLoad,">OUTS:%c,%c,%c,%c\r\n",buffer[7],buffer[8],buffer[9],buffer[10]);
					bufferDownLoad[15]=0x00;
					serial_send(bufferDownLoad,16);
					printf("%s",bufferDownLoad);
					
				}
			}

			buffer[n]=0x00;
		}

    		close(newfd);
		sleep(TIMESLEEPTCP);
	}

	//return NULL;
}

void* Serial_handle (void* message)
{
	
	int32_t result;
	int32_t datosin;
	result = serial_open(NPUERTOSERIAL,BAUDRATE);
	char bufferSerial[BUFFERMAXSIZE];
	char bufferUpLoad[BUFFERMAXSIZE];
	
	while(1)
	{
		sleep(TIMESLEEPREAD);//duermo el hilo para que el cpu haga otras cosas		
		datosin=serial_receive(bufferSerial,16);//leo el puerto serial y lo almaceno en el buffer 
				
		if(datosin==NBYTESINCOMINGSERIAL)
		{
			if(bufferSerial[0]=='>')
			{			
				uint32_t aux;				
				bufferSerial[datosin]=0x00;
				aux=bufferSerial[UARTBPULSADOR]-'0';
			
				if((aux>=0)&&(aux<=3))//compruebo que los valores recibidos sean correctos '0'-'3' 
				{
					datoUpLoad=bufferSerial[UARTBPULSADOR];
					sprintf(bufferUpLoad,":LINE%cTG\n",datoUpLoad);
			    		if (write (newfd, bufferUpLoad, 11) == -1)
			    		{
			      			perror("Error escribiendo mensaje en socket");
			      			exit (1);
			    		}
				}
				else
				{
					datoUpLoad=0x00;
				}
						
				printf ("BOTON PRESIONADO NRO: %c\n", datoUpLoad);
			}
		}
		
	}
	//return NULL;
}

int main(void)
{
	
	//Configuración de receptores de señales del sistema
	struct sigaction sa;
	
	sa.sa_handler = manejoInt;
	sa.sa_flags = 0; 
	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGINT,&sa,NULL)==-1)
	{
		perror("error en sigint");
	}

    struct sigaction se;
 	se.sa_handler = manejoTerm;
    se.sa_flags = 0; 
    sigemptyset(&se.sa_mask);

	   
	if(sigaction(SIGTERM,&se,NULL)==-1)
	{
		perror("error en sigint");
	}




	printf("Inicio Serial Service\r\n");
	
	

	const char *message1 = "Tcp";
	const char *message2 = "Serial";
	



	bloquearSign();//bloqueo las signasl
	//Creación de hilos

	if((pthread_create (&SerialHandle, NULL, Serial_handle, (void *) message1))!=0)
	{
		printf("Error en la creacion hilo %s","tcp");
		exit(0);
	}
	if((pthread_create (&TcpHandel, NULL, Tcp_handle, (void *) message2))!=0)
	{
		printf("Error en la creacion hilo %s","serial");
		exit(0);
	}
	desbloquearSign();//desbloqueo las signasl
	
	//Join de hilos
	if((pthread_join (SerialHandle, NULL))!=0)
	{
		printf("Error join %s","serial");
		exit(0);	
	}
	if((pthread_join (TcpHandel, NULL))!=0)
	{
		printf("Error join %s","tcp");
		exit(0);	
	}
	close(newfd);
	printf("SALIDA CORRECTA\n\r");

	exit(EXIT_SUCCESS);
	return 0;
}

void bloquearSign(void)
{
	sigset_t set;
 	int s;
 	sigemptyset(&set);
 	sigaddset(&set, SIGINT);
 	sigaddset(&set, SIGTERM);
 	pthread_sigmask(SIG_BLOCK, &set,NULL);
}

void desbloquearSign(void)
{
 	sigset_t set;
 	int s;
 	sigemptyset(&set);
 	sigaddset(&set, SIGINT);
 	sigaddset(&set, SIGTERM);
 	pthread_sigmask(SIG_UNBLOCK, &set,NULL);
}







