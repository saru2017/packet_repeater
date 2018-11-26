#include "watalib.h"

#define BUFSIZE 1024
#define MAX_CLIENTS 10
 
int g_clients[MAX_CLIENTS];


void watalab_list_serials()
{
  struct stat st;
  char filename[256];

  int ret;

  for(int i = 0; i < 20; i++){
    sprintf(filename, "/dev/ttyS%d", i);
    ret = stat(filename, &st);
    if(ret == 0){
      printf("%s\n",filename);
    }
  }
}


int watalab_open_serial(char *device)
{
  int i, fd;
  struct termios tty;
  char *c, buf[256];

  if((fd = open(device, O_RDWR|O_NOCTTY)) == -1){
    watalab_list_serials();
    exit(EXIT_FAILURE);
  }

  memset(&tty, 0, sizeof(tty));
  tty.c_cflag = CS8|CLOCAL|CREAD;
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 0;
  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &tty);

  return fd;
}


/***************************************************************/
void watalab_init_clients()
{
  int i;
 
  for(i = 0; i < MAX_CLIENTS; i++){
    g_clients[i] = 0;
  }
}
/***************************************************************/



/***************************************************************/
void watalab_set_fds(fd_set* pfds, int sock)
{
  int i;
 
  FD_ZERO(pfds);
  FD_SET(sock, pfds);
 
  for(i = 0; i < MAX_CLIENTS; i++){
    if(g_clients[i] == 1){
      FD_SET(i, pfds);
    }
  }
}
/***************************************************************/



/***************************************************************/
void watalab_add(int sock)
{
  if(sock < MAX_CLIENTS){
    g_clients[sock] = 1;
  }else{
    printf("connection overflow\n");
    exit(-1);
  }
}
/***************************************************************/


/***************************************************************/
void watalab_remove(int id)
{
  g_clients[id] = 0;
}
/***************************************************************/


/***************************************************************/
int watalab_get_max_sock()
{
  int i;
  int max_sock = 0;
 
  for(i = 0; i < MAX_CLIENTS; i++){
    if(g_clients[i] == 1){
      max_sock = i;
    }
  }
 
  return max_sock;
}
/***************************************************************/


/***************************************************************/
void watalab_broadcast(char* buf, int size, int from)
{
  int i;
 
  for(i = 0; i < MAX_CLIENTS; i++){
    if(i == from){
      continue;
    }
 
    if(g_clients[i] == 1){
      write(i, buf, size);
    }
  }
}
/***************************************************************/



/************************************************************/
int watalab_tcp_listen(int port)
{
  int sock;
  struct sockaddr_in addr;
  int yes = 1;
  int ret;
 
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("socket");
    exit(-1);
  }
 
  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if(ret < 0) {
    perror("bind");
    exit(-1);
  }
 
  ret = listen(sock, 5);
  if(ret < 0) {
    perror("listen");
    close(sock);
    exit(-1);
  }
  return sock;
}
/************************************************************/



/************************************************************/
int watalab_accept(int sock_listen){
  struct sockaddr addr;
  int len;
  int ret;
 
  len = sizeof(int);
  ret = accept(sock_listen, &addr, (socklen_t*) &len);
 
  return ret;
}
/************************************************************/



/************************************************************/
int watalab_tcp_connect(const char *hostname, int port)
{
  int sock;
  int ret;
  struct sockaddr_in addr;
  struct hostent *host;
 
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  addr.sin_family = AF_INET;
  host = gethostbyname(hostname);
  addr.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);
  addr.sin_port = htons(port);
  ret = connect(sock, (struct sockaddr *)&addr, sizeof addr);
  if(ret < 0){
    return -1;
  }else{
    return sock;
  }
}
/************************************************************/



void watalab_connect(char *server_name, int port)
{

}

void watalab_do_client(int fd, int sock)
{
  fd_set fds;
  int max_sock;
  int i;

  FD_ZERO(&fds);
  
  watalab_set_fds(&fds, fd);
  watalab_set_fds(&fds, sock);

  if(sock > fd){
    max_sock = sock;
  }else{
    max_sock = fd;
  }

  select(max_sock + 1, &fds, NULL, NULL, NULL);

  if(FD_ISSET(fd, &fds) != 0){
    char buf[1024];
    int size;

    size = read(fd, buf, 1024);
    buf[size] = '\x00';
    write(sock, buf, size);
  }


  if(FD_ISSET(sock, &fds) != 0){
    char buf[1024];
    int size;

    size = read(sock, buf, 1024);
    buf[size] = '\x00';
    write(fd, buf, size);
  }
}



int main(int argc, char** argv)
{
  int sock_listen;
  char serial_name[1024];
  char server_name[1024];
  int port;
  int fd, sock;

  if(argc != 4){
    printf("usage: %s [serial] [servername] [port]\n", argv[0]);
    exit(-1);
  }

  strcpy(serial_name, argv[1]);
  strcpy(server_name, argv[2]);
  port = atoi(argv[3]);


  fd = watalab_open_serial(serial_name);
  sock = watalab_tcp_connect(server_name, port);


  while(true){
    watalab_do_client(fd, sock);
  }
  close(fd);
  close(sock);
}
