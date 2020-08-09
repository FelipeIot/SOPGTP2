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

#define nbytesincomingserial 16
#define timeSleepRead 0.01
#define nPuertoSerial 1
#define baudrate 115200


char datoUpLoad;


void init_tcp(void);

void* Tcp_handle (void* message)
{
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[128];
	char bufferUpLoad[128];
	char bufferDownLoad[128];
	int newfd;
	int n;

	// Creamos socket
	int s = socket(AF_INET,SOCK_STREAM, 0);

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


		if( (n = read(newfd,buffer,128)) == -1 )
		{
			perror("Error leyendo mensaje en socket");
			exit(1);
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

		//printf("Recibi %d bytes.:%s\n",n,buffer);

		// Enviamos mensaje a cliente
		if(datoUpLoad!=0x00)
		{
			
			sprintf(bufferUpLoad,":LINE%cTG\n",datoUpLoad);
	    		if (write (newfd, bufferUpLoad, 11) == -1)
	    		{
	      			perror("Error escribiendo mensaje en socket");
	      			exit (1);
	    		}
			datoUpLoad=0x00;
		}

		// Cerramos conexion con cliente
    		close(newfd);
		sleep(0.1);
	}

	//return NULL;
}

void* Serial_handle (void* message)
{
	init_tcp();
	int32_t result;
	int32_t datosin;
	result = serial_open(nPuertoSerial,baudrate);
	char bufferSerial[128];
	
	while(1)
	{
		sleep(timeSleepRead);		
		datosin=serial_receive(bufferSerial,16);
		if(datosin==nbytesincomingserial)
		{
			if(bufferSerial[0]=='>')
			{			
				uint32_t aux;				
				bufferSerial[datosin]=0x00;
				aux=bufferSerial[14]-'0';
				if((aux>=0)&&(aux<=3))
				{
					datoUpLoad=bufferSerial[14];
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
	printf("Inicio Serial Service\r\n");
	pthread_t SerialHandle, TcpHandel;
	const char *message1 = "Tcp";
	const char *message2 = "Serial";
	






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
	
	exit(EXIT_SUCCESS);
	return 0;
}



void init_tcp(void)
{



}





