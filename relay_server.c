#include "watalib.h"

#define BUFSIZE 1024
#define MAX_CLIENTS 10
 
int g_clients[MAX_CLIENTS];


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



void watalab_do_server(int sock_listen)
{
  fd_set fds;
  int max_sock;
  int i;

  watalab_set_fds(&fds, sock_listen);
  max_sock = watalab_get_max_sock();

  if(max_sock < sock_listen){
    max_sock = sock_listen;
  }

  select(max_sock + 1, &fds, NULL, NULL, NULL);

  if(FD_ISSET(sock_listen, &fds) != 0){
    int sock;
    sock = watalab_accept(sock_listen);
    watalab_add(sock);
  }

  
  for(i = 0; i < MAX_CLIENTS; i++){
    if(g_clients[i] == 0){
      continue;
    }

    if(FD_ISSET(i, &fds) != 0){
      char buf[1024];
      int ret = recv(i, buf, 1024, 0);
      
      if(ret > 0){
	//	write(1, buf, ret);
	watalab_broadcast(buf, ret, i);
      }else{

      }
    }
  }
}



int main()
{
  int sock_listen;

  watalab_init_clients();
  sock_listen = watalab_tcp_listen(11111);
  while(true){
    watalab_do_server(sock_listen);
  }
  close(sock_listen);
}
