/*----------------showResult.c-----------------------*/
/*--SYSTEM PROGRAMMING MIDTERM PROJECT---------------*/
/*Author:Efkan DURAKLI-------------------------------*/
/*161044086------------------------------------------*/                
/*Date:16/04/2017------------------------------------*/       
/*---------------RESULT PROGRAM----------------------*/


#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "matrix.h"

#define RESULTFIFONAME "results.fifo"
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int r_close(int fildes);
ssize_t r_read(int fd, void *buf, size_t size);
ssize_t r_write(int fd, void *buf, size_t size);

/* clientdan sonuçları tek seferde okubilmek için struct*/
typedef struct {
  pid_t pid;
  double result1;
  double result2;
  long timeElapsed1;
  long timeElapsed2;
} total_t;

pid_t serverPid = 0;
int fdLog = 0;

/* SIGINT ve SIGUSR1 için signal handler*/
static void signal_handler (int signo) {

	int i = 0;
  	sigset_t sigset;
  	char str[25];

  	sigemptyset(&sigset);
  	sigaddset(&sigset, SIGUSR1);

  	kill(serverPid, SIGUSR1);
  	if (signo == SIGINT) {
  		sigprocmask(SIG_BLOCK, &sigset, NULL);
    	fprintf(stderr, "Ctrl+C sinyali aılndı.Program kapanıyor...\n");
    	sprintf(str, "Ctrl+C sinyali aılndı.Program kapanıyor...\n");
    	r_write(fdLog, str, (int)strlen(str)*sizeof(char));
    	r_close(fdLog);
  	}
  	else if (signo == SIGUSR1) {
    	fprintf(stderr, "Kill sinyali aılndı.Program kapanıyor...\n");
    	sprintf(str, "Kill sinyali aılndı.Program kapanıyor...\n");
    	r_write(fdLog, str, (int)strlen(str)*sizeof(char));
    	r_close(fdLog);
  	}
  	unlink(RESULTFIFONAME);
  	unlink("tempResult.text");
  	exit(0);
}

int main(void) {
	
  	int fdFifo = 0;
  	int fdTemp = 0;
  	pid_t tempPid = 0;
  	pid_t myPid = 0;
  	double result1 = 0;
  	double result2 = 0;
  	long timeElapsed1 = 0;
  	long timeElapsed2 = 0;
  	char str[100];
  	total_t totalbytes;

  	if ((fdTemp = open("tempResult.text", O_CREAT | O_WRONLY, FIFO_PERMS)) == -1) {
    	fprintf(stderr, "Failed to open tempfile for write\n");
    	return 1;
  	}
  	myPid = getpid();
  	r_write(fdTemp, &myPid, sizeof(pid_t));
  	r_close(fdTemp);

  	if ((fdLog = open("logs/Results.log", O_CREAT | O_WRONLY, FIFO_PERMS)) == -1) {
    	fprintf(stderr, "Failed to open tempfile for write\n");
    	return 1;
  	}

  	if ((fdFifo = open(RESULTFIFONAME, O_RDONLY)) == -1) {
    	perror("Client bulunamadı.\n");
    	return 1;
  	}

  	if ((fdTemp = open("tempfile.txt", O_RDONLY)) == -1) {
    	fprintf(stderr, "Failed to open server tempfile for read : %s\n", strerror(errno));
    	return 1;
  	}
  	r_read(fdTemp, &serverPid, sizeof(pid_t));
  	r_close(fdTemp);

   	struct sigaction act;
   	act.sa_handler = signal_handler;
   	act.sa_flags = 0;
   	if ((sigemptyset(&act.sa_mask) == -1) ||
       	(sigaction(SIGINT, &act, NULL) == -1) ||
       	(sigaction(SIGUSR1, &act, NULL) == -1)) {
    	perror("Failed to set signal handler.");
    	return 1;
   	}

   	fprintf(stderr, "Pid of Client\t\tResult1\t\t\tResult2\n");
 	
 	/* program dışarıdan bir sinyal gelmediği sürece yaşamaya devam eder
 	   client eklendikçe sonuçları fifodan okur*/
   	for ( ; ; ) {
   		
      	r_read(fdFifo, &totalbytes, 36*sizeof(char));
      	sprintf(str, "Matrix, Pid of client = %ld\n",(long)totalbytes.pid);
      	r_write(fdLog, str, (int)strlen(str)*sizeof(char));
      	fprintf(stderr, "%ld\t\t\t", (long)totalbytes.pid);
      	fprintf(stderr, "%.5f\t\t", totalbytes.result1);
      	sprintf(str, "Result1 = %f\t\t%ld miliseconds elapsed\n", totalbytes.result1, totalbytes.timeElapsed1);
      	r_write(fdLog, str, (int)strlen(str)*sizeof(char));
      	sprintf(str, "Result2 = %f\t\t%ld miliseconds elapsed\n\n", totalbytes.result2, totalbytes.timeElapsed2);
      	fprintf(stderr, "%.5f\t\n", totalbytes.result2);
      	r_write(fdLog, str, (int)strlen(str)*sizeof(char));	
   	}
}

/* aşağıdaki fonksiyonlar daha güvenli oldukları için kitaptan alınmıştır.*/
int r_close(int fildes) {
   int retval;
   while (retval = close(fildes), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t r_read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t r_write(int fd, void *buf, size_t size) {
   char *bufp;
   size_t bytestowrite;
   ssize_t byteswritten;
   size_t totalbytes;

   for (bufp = buf, bytestowrite = size, totalbytes = 0;
        bytestowrite > 0;
        bufp += byteswritten, bytestowrite -= byteswritten) {
      byteswritten = write(fd, bufp, bytestowrite);
      if ((byteswritten) == -1 && (errno != EINTR))
         return -1;
      if (byteswritten == -1)
         byteswritten = 0;
      totalbytes += byteswritten;
   }
   return totalbytes;
}
