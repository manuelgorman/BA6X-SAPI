#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

#include "../include/hidapi.h"
#include "../include/common.h"

void handle_read(int confd);
void * doit(void * arg);

/*
 * Communication pour BA6X
 * print|UnMessageLigne1|UnMessageLigne2
 * product|Designation|Poids|Prix\Qte
 * clean
*/

int main(int argc, char * argv[]) {

	int fd, confd, sockfd;
	struct sockaddr_in addr;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_len;
	pthread_t tid;

	memset(&addr, 0, sizeof(addr));
	memset(&cli_addr, 0, sizeof(cli_addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	addr.sin_port = htons(SERVER_PORT);

	if( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stdout, "<Serveur> Impossible de monter un socket sur l'interface %s\n", SERVER_ADDR);
		exit(-1);
	}else{
		fprintf(stdout, "<Serveur> Initialisation du socket sur %s\n", SERVER_ADDR);
	}

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		fprintf(stdout, "<Serveur> L'interface %s:%i est impossible d'access !\n", SERVER_ADDR, SERVER_PORT);
		exit(-2);
	}

	if(listen(fd, BACKLOG) == -1) {
		printf("error to listen the socket\n");
		exit(-3);
	}else{
		fprintf(stdout, "<Serveur> Ecoute sur %s:%i\n", SERVER_ADDR, SERVER_PORT);
	}

  /* Initialisation de la liaison BA6X HID USB */
  if (!initializeDevice()) {
    fprintf(stderr, "<Pilote> Impossible de se connecter sur le BA63 (USB) !\n");
    exit(-4);
  }

  sendBuffer(display, SEQ_CHARSET);

	while(1) {
		confd = accept(fd, NULL, NULL);
		pthread_create(&tid, NULL, &doit, (void*)&confd);
	}

  /* Libére l'affichage */
  freeDevice();
}

void * doit(void * arg) {

	int confd = *(int*)arg;
	pthread_detach(pthread_self());
	handle_read(confd);
	close(confd);
	return NULL;

}

void handle_read(int confd) {

	unsigned char buf[BA6X_BYTES];
	int num = 0;

	fprintf(stdout, "<Serveur> Lecture de buffer\n");

	while( (num = read(confd, buf, BA6X_BYTES)) > 0) {

		fprintf(stdout, "<Serveur> Client %i demande '%s'..\n", confd, buf);

    pthread_mutex_lock(&displayLock);

    sendBuffer(display, SEQ_CLEAR); //Nettoyage
    sendBuffer(display, SEQ_CURSOR); //Positionnement du curseur
    sendBuffer(display, buf); //Le buffer

    pthread_mutex_unlock(&displayLock);

		if(write(confd, buf, num) == -1)
		{
			fprintf(stdout, "<Serveur> Impossible d'ecrire sur le buffer de sortie\n");
			break;
		}

    memset(buf, 0, BA6X_BYTES);

	}

	if(num == 0) {
		fprintf(stdout, "<Serveur> Le client %i a mis fin a la communication\n", confd);
	}
	else if(num < 0 && errno == EINTR)
	{
		fprintf(stdout, "<Serveur> Le client %i a mis fin a la transmission, timeout ?\n", confd);
	}
	else if(num < 0) {
		fprintf(stdout, "<Serveur> La trame de connexion est mauvaise, refus.\n");
	}
}
