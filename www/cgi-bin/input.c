#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <string.h>
#define SIZE	30
#define SSIZE   SIZE+1
int main(void)
{
char buffer[SSIZE];
strcpy(buffer, getenv("QUERY_STRING"));
printf("Content-Type: text/html; charset=UTF-8\n\n\n <html>%s</html>", buffer);
  
strcpy((char *)shmat(shmget(9998, SIZE, IPC_CREAT | 0666), NULL, 0),buffer);
return 0;
}
