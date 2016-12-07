#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>




int tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ){
    perror("alloc tun");
    exit(1);
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
   *        IFF_TAP   - TAP device  
   *
   *        IFF_NO_PI - Do not provide packet information  
   */ 
  ifr.ifr_flags = IFF_TUN|IFF_NO_PI; 
  if( *dev )
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}      


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
	
/*

int main (int argc, char** argv){
	int tun,i;
	char buffer[1024];
    if(argc==1)
    {
         fprintf(stderr,"Mettre un argument\n");
         printf("Mettre un argument\n");
         return 1;
    }
    strcpy(buffer,argv[1]);
    tun=tun_alloc(buffer);
   // printf("tun :%d\n", tun);
    while(1)
    {
		iftun(tun , 1);
	}
    
  return 0;
}*/
