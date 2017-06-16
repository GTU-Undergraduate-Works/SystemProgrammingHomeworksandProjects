/*----------------seeWhat.c--------------------------*/
/*--SYSTEM PROGRAMMING MIDTERM PROJECT---------------*/
/*Author:Efkan DURAKLI-------------------------------*/
/*161044086------------------------------------------*/                
/*Date:16/04/2017------------------------------------*/ 			
/*---------------CLIENT PROGRAM----------------------*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "matrix.h"

#define STRSIZE 25
#define MAX_SIZE 50
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define RESULTFIFONAME "results.fifo"

/* showresult programına sonuçları tek seferde göndermek için struct*/
typedef struct {
	pid_t pid;
	double result1;
	double result2;
	long timeElapsed1;
	long timeElapsed2;
} total_t;

int r_close(int fildes);
ssize_t r_read(int fd, void *buf, size_t size);
pid_t r_wait(int *stat_loc);
ssize_t r_write(int fd, void *buf, size_t size);

pid_t serverPid = 0;
pid_t resultPid = 0;
int isParent = 1;

/* SIGINT ve SIGUSR1 için signal handler */
static void signal_handler (int signo) {

  	int i = 0;
  	int status;
  	pid_t wpid;
  	char str[STRSIZE];
  	sigset_t sigset;

  	sigemptyset(&sigset);
  	sigaddset(&sigset, SIGUSR1);

  	if (isParent) {
    	kill(serverPid, SIGUSR1);
    	kill(resultPid, SIGUSR1);
    if (signo == SIGINT) {
        sigprocmask(SIG_BLOCK, &sigset, NULL);
      	fprintf(stderr, "Ctrl+C sinyali aılndı.Client kapanıyor...\n");
    }
    else if (signo == SIGUSR1)
      	fprintf(stderr, "Kill sinyali aılndı.Client kapanıyor...\n");
    while ((wpid = r_wait(&status)) != -1);
    sprintf(str, "%ld.fifo", (long)getpid());
    unlink(str);
    unlink(RESULTFIFONAME);
  	}
  	exit(0);
}

int main(int argc, char *argv[]) {

   	int i = 0,j = 0;
   	int size = 0;
   	int flag = 0;
   	int serverFd = 0;
   	int resultfd = 0;
   	int clientfd = 0;
   	int tempFd = 0;
   	int counter = 0;
   	int status = 0;
   	int fd[2];
   	pid_t myPid = 0;
   	pid_t childpid = 0;
   	pid_t wpid = 0;
   	double determinant = 0;
   	double result1 = 0, result2 = 0;
   	double shifted[20][20] ;
   	double convolution[20][20];
   	double arr[20][20];
   	char fileName[20];
   	char clienFifoName[MAX_SIZE];
   	struct timeval end;
   	struct timeval start;
   	long timeElapsed1;
   	long timeElapsed2;
   	long timedif;
   	FILE *logFile;
   	total_t totalbytes;

   	myPid = getpid();
   	if (argc != 2) {
   		fprintf(stderr, "Usage: %s <mainpipename>\n", argv[0]);
   		return 1;
   	}
   	if ((tempFd = open("tempfile.txt", O_RDONLY)) == -1) {
   		fprintf(stderr, "Server %s bulunamadı.\n",argv[1]);
   		return 1;
   	}
   	r_read(tempFd, &serverPid, sizeof(pid_t));
   	r_read(tempFd, &size, sizeof(int));
   	r_close(tempFd);

   	if ((serverFd = open(argv[1], O_WRONLY)) == -1) {
   		fprintf(stderr, "Client failed to open main FIFO\n");
   		return 1;
   	}

   	sprintf(clienFifoName, "%ld.fifo", (long)myPid);
   	if ((mkfifo(clienFifoName, FIFO_PERMS) == -1) && (errno != EEXIST)) {
    	perror("Client failed to create a FIFO");
    	return 1;
   	}

   	/* request için servera SIGUSR2 sinayli gönderilir ve ana fifoya client pid'sini yazar */
   	kill(serverPid, SIGUSR2);
   	r_write(serverFd, &myPid, sizeof(pid_t));

   	if ((clientfd = open(clienFifoName, O_RDONLY)) == -1) {
    	fprintf(stderr, "Client failed to open tempfile: %s\n", strerror(errno));
    	return 1;
   	}

   	if ((mkfifo(RESULTFIFONAME, FIFO_PERMS) == -1) && (errno != EEXIST)) {
    	perror("Client failed to create a FIFO");
    	return 1;
   	}

   	if ((resultfd = open(RESULTFIFONAME, O_RDWR)) == -1) {
    	fprintf(stderr, "Client failed to open tempfile: %s\n", strerror(errno));
    	return 1;
   	}

   	struct sigaction act;
   	act.sa_handler = signal_handler;
   	act.sa_flags = 0;
   	if ((sigemptyset(&act.sa_mask) == -1) ||
       	(sigaction(SIGINT, &act, NULL) == -1) ||
       	(sigaction(SIGUSR1, &act, NULL) == -1)) {
    	perror("Failed to set signal handler.");
    	return 1;
   	}



   	/* dışarıdan sinyal gelmediği sürece client çalışmaya devam edecek */
   	for ( ; ; ) {
		/* client sürekli request gönderir servera */
    	kill(serverPid, SIGUSR2);
    	counter++;

   		if (pipe(fd) == -1) {
   			fprintf(stderr, "Failed to create pipe.\n");
   			return 1;
   		}
    	for (i = 0; i < size; i++) {
      		for (j = 0; j < size; j++)
        		r_read(clientfd, &arr[i][j], sizeof(double));
    	}
    	sprintf(fileName, "logs/%ld-%d.log", (long)myPid, counter);
    	if ((logFile = fopen(fileName, "a")) == NULL) {
      		fprintf(stderr, "Client failed to open\n");
    	}
    	if (flag == 0) {
        	if ((tempFd = open("tempResult.text", O_RDONLY)) != -1) {
          		flag = 1;
          		r_read(tempFd, &resultPid, sizeof(pid_t));
          		r_close(tempFd);
        	}
    	}
    	childpid = fork();
    	if (childpid == -1) {
      		fprintf(stderr, "Failed to create fork\n");
      		return 1;
    	}
    	if (childpid == 0) {
      		r_close(fd[0]);
      		gettimeofday(&start, NULL);
      		isParent = 0;
      		shiftedInverseMatrix(arr, shifted, size);
      		fprintf(logFile, "Shifted Inverse matrix\n\n");
      		for (i = 0; i < size; i++) {
        		for (j = 0; j < size; j++) 
          			fprintf(logFile, "%.5f  ", shifted[i][j]);
      			fprintf(logFile, "\n");
      		}
      		result1 = determinantOfMatrix(arr, size) - determinantOfMatrix(shifted, size);
      		gettimeofday(&end, NULL);
      		timedif = 1000*(end.tv_sec - start.tv_sec) +
					 	   (end.tv_usec - start.tv_usec)/1000.0;
      		r_write(fd[1], &result1, sizeof(double));
      		r_write(fd[1], &timedif, sizeof(long));
      		r_close(fd[1]);
	    	exit(1);

    	} else {
      		while(wpid = r_wait(&status) != -1);
      		childpid = fork();
      		if (childpid == -1) {
        		fprintf(stderr, "Failed to create fork\n");
        		return 1;
      		}
      		if (childpid == 0) {
      			r_close(fd[0]);
      			gettimeofday(&start, NULL);
       	 		isParent = 0;
        		convolutionOfMatrix(arr, convolution, size);
        		fprintf(logFile, "\nConvolution matrix\n\n");
        		for (i = 0; i < size; i++) {
          		for (j = 0; j < size; j++) 
            		fprintf(logFile, "%.5f  ", convolution[i][j]);
        		fprintf(logFile, "\n");
        		}
        		result2 = determinantOfMatrix(arr, size)- determinantOfMatrix(convolution, size);
        		gettimeofday(&end, NULL);
        		timedif = 1000*(end.tv_sec - start.tv_sec) +
					   		   (end.tv_usec - start.tv_usec)/1000.0;
        		r_write(fd[1], &result2, sizeof(double));
        		r_write(fd[1], &timedif, sizeof(long));
        		r_close(fd[1]);
        		exit(2);
      		}
    		fprintf(logFile, "\nOriginal matrix\n\n");
    		for (i = 0; i < size; i++) {
      			for (j = 0; j < size; j++) 
        			fprintf(logFile, "%.5f  ", arr[i][j]);
      			fprintf(logFile, "\n");
    		} 
    		while(wpid = r_wait(&status) != -1);
    		fclose(logFile);
    		r_close(fd[1]);
    		r_read(fd[0], &result1, sizeof(double));
    		r_read(fd[0], &timeElapsed1, sizeof(long));
    		r_read(fd[0], &result2, sizeof(double));
    		r_read(fd[0], &timeElapsed2, sizeof(long));
    		r_close(fd[0]);

    		totalbytes.pid = myPid;
    		totalbytes.result1 = result1;
    		totalbytes.result2 = result2;
    		totalbytes.timeElapsed1 = timeElapsed1;
    		totalbytes.timeElapsed2 = timeElapsed2;
    		r_write(resultfd, &totalbytes, 36*sizeof(char));
    	}
	}
}

/* aşağıdaki fonksiyonlar kitaptan alınmıştır.*/
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

pid_t r_wait(int *stat_loc) {
   pid_t retval;
   while (((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
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




