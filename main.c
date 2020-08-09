#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "SerialManager.h"



void* Tcp_handle (void* message)
{
	while(1)
	{	
		printf ("%s\n", (const char *) message);
		sleep(1);
	}
	//return NULL;
}

void* Serial_handle (void* message)
{
	while(1)
	{
	
		printf ("%s\n", (const char *) message);
		sleep(1);
	}
	//return NULL;
}

int main(void)
{
	printf("Inicio Serial Service\r\n");
	pthread_t SerialHandle, TcpHandel;
	const char *message1 = "Serial";
	const char *message2 = "Tcp";
	























	if((pthread_create (&SerialHandle, NULL, Tcp_handle, (void *) message1))!=0)
	{
		printf("Error en la creacion hilo %s","tcp");
		exit(0);
	}
	if((pthread_create (&TcpHandel, NULL, Serial_handle, (void *) message2))!=0)
	{
		printf("Error en la creacion hilo %s","serial");
		exit(0);
	}

	pthread_join (SerialHandle, NULL);
	pthread_join (TcpHandel, NULL);
	
	exit(EXIT_SUCCESS);
	return 0;
}
