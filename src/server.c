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
#include <signal.h>

#include "../include/hidapi.h"
#include "../include/common.h"

int fd, confd, sockfd;

void handle_read(int confd);
void * doit(void * arg);

/*
 * Communication pour BA6X
 * print|UnMessageLigne1|UnMessageLigne2
 * product|Designation|Poids|Prix\Qte
 * clean
*/

int main(int argc, char * argv[]) {

	struct sockaddr_in addr;
	struct sockaddr_in cli_addr;
	//socklen_t cli_addr_len;
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

  /* Libére l'affichage (BA6x USB) */
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

	unsigned char buf[MAX_STR];
	unsigned char tronq[BA6X_LINE_MAX];
	int num = 0, nParam = 0, i = 0, j =0;

  unsigned char **pch = calloc(5, sizeof(unsigned char*));

	fprintf(stdout, "<Serveur> Lecture de buffer\n");

	while( (num = read(confd, buf, MAX_STR)) > 0) {

    /* Telnet */
    if (DEBUG) replace(buf, '\n', '\0');
    if (DEBUG) replace(buf, 0xd, '\0');
    if (DEBUG) replace(buf, 0xa, '\0');

		fprintf(stdout, "<Serveur> Client %i demande '%s'..\n", confd, buf);

    nParam = extractParams(buf, pch);

    /*
     * Communication pour BA6X
     * print|UnMessageLigne1|UnMessageLigne2
     * product|Designation|Poids|Prix\Qte
     * clean
     * exit
    */

    if (!strcmp(pch[0], "print")) {

      if (nParam < 2) {

        fprintf(stderr, "<Pilote> Mauvais nombre de paramètres pour 'clean', %i obtenu(s), 1 demandé(s) !\n", nParam-1);

        if(write(confd, FAILED_PARAMS, strlen(FAILED_PARAMS)+1) == -1)
        {
          fprintf(stdout, "<Serveur> Impossible d'ecrire sur le buffer de sortie\n");
          break;
        }

      }else{

        pthread_mutex_lock(&displayLock);

        sendBuffer(display, SEQ_CLEAR); //Nettoyage
        setCursor(display, 1, 0); //Positionnement du curseur

				//On vérifie la taille du texte pour tonquer
				if (strlen(pch[1]) > BA6X_LINE_MAX) {
					if (DEBUG) fprintf(stdout, "<Pilote> Taille de ligne 1 trop grande, tronquage..\n");
					tronquer(pch[1], tronq, BA6X_LINE_MAX);
					sendBuffer(display, tronq); //Le buffer
				}else{
					sendBuffer(display, pch[1]); //Le buffer
				}

				if (nParam == 3) {
					setCursor(display, 2, 0); //Positionnement du curseur

					if (strlen(pch[1]) > BA6X_LINE_MAX) {
						if (DEBUG) fprintf(stdout, "<Pilote> Taille de ligne 2 trop grande, tronquage..\n");
						tronquer(pch[2], tronq, BA6X_LINE_MAX);
						sendBuffer(display, tronq); //Le buffer
					}else{
						sendBuffer(display, pch[2]); //Le buffer
					}

        }

        pthread_mutex_unlock(&displayLock);

      }

    }else if(!strcmp(pch[0], "clean")) {

      if (nParam != 1) {
        fprintf(stderr, "<Pilote> Mauvais nombre de paramètres pour 'clean', %i obtenu(s), 0 demandé(s) !\n", nParam-1);

        if(write(confd, FAILED_PARAMS, strlen(FAILED_PARAMS)+1) == -1)
        {
          fprintf(stdout, "<Serveur> Impossible d'ecrire sur le buffer de sortie\n");
          break;
        }

      }else{

        pthread_mutex_lock(&displayLock);
        sendBuffer(display, SEQ_CLEAR); //Nettoyage
        setCursor(display, 0, 0); //Positionnement du curseur
        pthread_mutex_unlock(&displayLock);

      }

    }else if(!strcmp(pch[0], "exit")) { /* Exit client handler */
      break;
    }else{ //No command match

      fprintf(stderr, "<Pilote> Commande inconnue: %s\n", pch[0]);

      if(write(confd, UNKNOWN_COMMAND, strlen(UNKNOWN_COMMAND)+1) == -1)
      {
        fprintf(stdout, "<Serveur> Impossible d'ecrire sur le buffer de sortie\n");
        break;
      }

    }

    memset(buf, 0, MAX_STR);

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
    exit(-1);
	}
}

int extractParams(unsigned char *src, unsigned char **dst) {
  int nParam = 0, cursor = 0;
  unsigned long i = 0;

  fprintf(stdout, "<Debug> Debut extraction des parametres sur (%s)..\n", src);

  for (i = 0; i < NB_PARAMS; i++) {
    if (dst[i]) {
      fprintf(stdout, "<Info> Libération d'espace mémoire sur **dst !\n");
      free(dst[i]);
      dst[i] = NULL;
    }
  }

  for (i = 0; i < strlen(src)+1; i++) {
    if (src[i] == '|' || src[i] == '\0') {

      dst[nParam] = calloc((i-cursor)+1, sizeof(unsigned char));

      memcpy(dst[nParam], src+cursor, i-cursor);
      dst[nParam][(i-cursor)+1] = '\0';

      if (DEBUG) fprintf(stdout, "<Debug> Allocation d'un parametre (i = %lu; cursor = %d) de taille %lu avec '%s'\n", i, cursor, i-cursor+1, dst[nParam]);
      nParam++;
      cursor = i+1;
      if(src[i] == '\0') break;
    }
  }

  return nParam;
}

void replace(unsigned char *src, unsigned char occ, unsigned char new) {
  unsigned long i = 0;
  for (i = 0; i < strlen(src); i++) {
    if (src[i] == occ) {
      src[i] = new;
    }
  }
}

void tronquer(unsigned char *src, unsigned char *dst, int tailleMax) {
	int i = 0;
	
	for (i = 0; i < tailleMax-3; i ++) {
		dst[i] = src[i];
	}
	dst[i] = '.';
	dst[i+1] = '.';
	dst[i+2] = '.';
}
