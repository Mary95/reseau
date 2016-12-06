#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

/* taille maximale des lignes */
#define MAXLIGNE 80
#define CIAO "Au revoir ...\n"


int iftun(int src, int dest)
{
	char buffer[2048];
	ssize_t recup = read(src,buffer,2048);
	if(recup<0)
	{
		perror("read");
		exit(1);
	}
	recup = write(dest,buffer,2048);
	if(recup<0)
	{
		perror("write");
		exit(1);
	}
	return 0;
}	

int ext_in()
{
	  int s,n; /* descripteurs de socket */
  int len,on; /* utilitaires divers */
  struct addrinfo * resol; /* résolution */
  struct addrinfo indic = {AI_PASSIVE, /* Toute interface */
                           PF_INET6,SOCK_STREAM,0, /* IP mode connecté */
                           0,NULL,NULL,NULL};
  struct sockaddr_in6 client; /* adresse de socket du client */
  char * port; /* Port pour le service */
  int err; /* code d'erreur */
  
  /* Traitement des arguments */
  if (argc!=2) { /* erreur de syntaxe */
    printf("Usage: %s port\n",argv[0]);
    exit(1);
  }
  
  port=argv[1]; fprintf(stderr,"Ecoute sur le port %s\n",port);
  err = getaddrinfo(NULL,port,&indic,&resol); 
  if (err<0){
    fprintf(stderr,"Résolution: %s\n",gai_strerror(err));
    exit(2);
  }

  /* Création de la socket, de type TCP / IP */
  if ((s=socket(resol->ai_family,resol->ai_socktype,resol->ai_protocol))<0) {
    perror("allocation de socket");
    exit(3);
  }
  fprintf(stderr,"le n° de la socket est : %i\n",s);

  /* On rend le port réutilisable rapidement /!\ */
  on = 1;
  if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))<0) {
    perror("option socket");
    exit(4);
  }
  fprintf(stderr,"Option(s) OK!\n");

  /* Association de la socket s à l'adresse obtenue par résolution */
  if (bind(s,resol->ai_addr,sizeof(struct sockaddr_in6))<0) {
    perror("bind");
    exit(5);
  }
  freeaddrinfo(resol); /* /!\ Libération mémoire */
  fprintf(stderr,"bind!\n");

  /* la socket est prête à recevoir */
  if (listen(s,SOMAXCONN)<0) {
    perror("listen");
    exit(6);
  }
  fprintf(stderr,"listen!\n");

  while(1) {
    /* attendre et gére
     * r indéfiniment les connexions entrantes */
    len=sizeof(struct sockaddr_in6);
    if( (n=accept(s,(struct sockaddr *)&client,(socklen_t*)&len)) < 0 ) {
      perror("accept");
      exit(7);
    }
    /* Nom réseau du client */
    char hotec[NI_MAXHOST];  char portc[NI_MAXSERV];
    err = getnameinfo((struct sockaddr*)&client,len,hotec,NI_MAXHOST,portc,NI_MAXSERV,0);
    if (err < 0 ){
      fprintf(stderr,"résolution client (%i): %s\n",n,gai_strerror(err));
    }else{ 
      fprintf(stderr,"accept! (%i) ip=%s port=%s\n",n,hotec,portc);
    }
    /* traitement */
    iftun(tun , 1);
  }
  return EXIT_SUCCESS;
	
}

int ext_out(int argc, char *argv[])
{
  char * hote; /* nom d'hôte du  serveur */   
  char * port; /* port TCP du serveur */   
  char ip[NI_MAXHOST]; /* adresse IPv4 en notation pointée */
  struct addrinfo *resol; /* struct pour la résolution de nom */
  int s; /* descripteur de socket */

  /* Traitement des arguments */
  if (argc!=3) {/* erreur de syntaxe */
    printf("Usage: %s hote port\n",argv[0]);
    exit(1);
  }
  hote=argv[1]; /* nom d'hôte du  serveur */   
  port=argv[2]; /* port TCP du serveur */   

  /* Résolution de l'hôte */
  if ( getaddrinfo(hote,port,NULL, &resol) < 0 ){
    perror("résolution adresse");
    exit(2);
  }

  /* On extrait l'addresse IP */
  sprintf(ip,"%s",inet_ntop(AF_INET6,((struct sockaddr_in6*)resol->ai_addr)->sin_addr));

  /* Création de la socket, de type TCP / IP */
  /* On ne considère que la première adresse renvoyée par getaddrinfo */
  if ((s=socket(resol->ai_family,resol->ai_socktype, resol->ai_protocol))<0) {
    perror("allocation de socket");
    exit(3);
  }
  fprintf(stderr,"le n° de la socket est : %i\n",s);

  /* Connexion */
  fprintf(stderr,"Essai de connexion à %s (%s) sur le port %s\n\n",
	  hote,ip,port);
  if (connect(s,resol->ai_addr,sizeof(struct sockaddr_in))<0) {
    perror("connexion");
    exit(4);
  }
  freeaddrinfo(resol); /* /!\ Libération mémoire */

  /* Session */
  char tampon[MAXLIGNE + 3]; /* tampons pour les communications */
  ssize_t lu;
  int fini=0;
  while( 1 ) { 
    /* Jusqu'à fermeture de la socket (ou de stdin)     */
    /* recopier à l'écran ce qui est lu dans la socket  */
    /* recopier dans la socket ce qui est lu dans stdin */

    /* réception des données */
    lu = recv(s,tampon,MAXLIGNE,0); /* bloquant */
    if (lu == 0 ) {
      fprintf(stderr,"Connexion terminée par l'hôte distant\n");
      break; /* On sort de la boucle infinie */
    }
    tampon[lu] = '\0';
    printf("reçu: %s",tampon);
    if ( fini == 1 )
      break;  /* on sort de la boucle infinie*/
    
    /* recopier dans la socket ce qui est entré au clavier */    
    if ( fgets(tampon,MAXLIGNE - 2,stdin) == NULL ){/* entrée standard fermée */
      fini=1;
      fprintf(stderr,"Connexion terminée !!\n");
      fprintf(stderr,"Hôte distant informé...\n");
      shutdown(s, SHUT_WR); /* terminaison explicite de la socket 
			     dans le sens client -> serveur */
      /* On ne sort pas de la boucle tout de suite ... */
    }else{   /* envoi des données */
      send(s,tampon,strlen(tampon),0);
    }
  } 
  /* Destruction de la socket */
  close(s);

  fprintf(stderr,"Fin de la session.\n");
  return EXIT_SUCCESS;
}
