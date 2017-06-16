/*----------------timerServer.c----------------------*/
/*--SYSTEM PROGRAMMING MIDTERM PROJECT---------------*/
/*Author:Efkan DURAKLI-------------------------------*/
/*161044086------------------------------------------*/                
/*Date:16/04/2017------------------------------------*/       
/*---------------SERVER PROGRAM----------------------*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "matrix.h"

#define STRSIZE 25
#define MAX_SIZE 50
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int r_close(int fildes);
ssize_t r_read(int fd, void *buf, size_t size);
pid_t r_wait(int *stat_loc);
ssize_t r_write(int fd, void *buf, size_t size);

char mainpipename[MAX_SIZE];
char clientFifoName[MAX_SIZE];
pid_t clientPids[250];
int numberOfClient = 0;
int isParent = 1;
int fdLog = 0;

static volatile sig_atomic_t flag = 0;

/* clientdan gelen request sinyali için signal handler*/
static void request_handler(int signo) {
	flag = 1;
}

/* SIGINT ve SIGUSR1 için signal handler*/
static void signal_handler (int signo) {

	int i = 0;
	int status;
	pid_t wpid;
	char str[STRSIZE];
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);

	if (isParent) {
		if (signo == SIGINT) {
      		sigprocmask(SIG_BLOCK, &sigset, NULL);
			fprintf(stderr, "Ctrl+C sinyali alınıd.Server kapanıyor...\n");
    	}
		else if (signo == SIGUSR1) 
			fprintf(stderr, "Kill sinyali alındı.Server kapanıyor...\n");

		for (i = 0; i < numberOfClient; i++)
			kill(clientPids[i], SIGUSR1);
		while ((wpid = r_wait(&status)) != -1);
		if (signo == SIGINT) {
      		sprintf(str, "Ctrl+C sinyali aılndı.Server kapanıyor...\n");
      		r_write(fdLog, str, (int)strlen(str)*sizeof(char));
      		r_close(fdLog);
		} else {
			
      		sprintf(str, "Kill sinyali aılndı.Server kapanıyor...\n");
      		r_write(fdLog, str, (int)strlen(str)*sizeof(char));
      		r_close(fdLog);
		}
	}
	unlink(mainpipename);
	unlink("tempfile.txt");
	exit(0);
}

int main(int argc, char *argv[]) {

	int tempFd = 0;
  	pid_t myPid = 0;
  	pid_t clientPid = 0;
  	pid_t childpid = 0;
  	int serverFd = 0;
  	int clientFd = 0;
  	int i = 0, j = 0;
  	int size = 0;
  	char str[100];
  	double arr[20][20];
  	double determinat = 0.0;
  	struct timeval end;
  	struct timeval start;
  	long timedif;
  	long miliseconds;

  	if (argc != 4) {
		fprintf(stderr, "Usage: %s <ticks in miliseconds> <n> <mainpipename>\n", argv[0]);
		return 1;
  	}
  	miliseconds = (long)1000*atoi(argv[1]);
  	size = 2*atoi(argv[2]);
  	if (size > 20 && size < 2) {
  		fprintf(stderr, "n 1 ile 10 arasında olamalı.\n");
  		return 1;
  	}
  	strcpy(mainpipename, argv[3]);
  	if ((tempFd = open("tempfile.txt", O_CREAT | O_WRONLY, FIFO_PERMS)) == -1) {
  		fprintf(stderr, "Server failed to open tempfile for write : %s\n", strerror(errno));
  		return 1;
  	}
  	myPid = getpid();
  	r_write(tempFd, &myPid, sizeof(pid_t));
  	r_write(tempFd, &size, sizeof(int));
  	close(tempFd); 
  	/* log dosyalarının oluşacağı klasör oluşturulur.*/
  	mkdir("logs", 0700); 

  	struct sigaction act;
  	act.sa_handler = signal_handler;
  	act.sa_flags = 0;
  	if ((sigemptyset(&act.sa_mask) == -1) || 
  	  	(sigaction(SIGINT, &act, NULL) == -1) ||
  	  	(sigaction(SIGUSR1, &act, NULL) == -1)) {
  		perror("Failed to set Signal handler.");
  		return 1;
  	}

  	struct sigaction act1;
  	act1.sa_handler = request_handler;
  	act1.sa_flags = 0;
  	if ((sigemptyset(&act1.sa_mask) == -1) ||
  	  	(sigaction(SIGUSR2, &act1, NULL) == -1)) {
  		perror("Failed to set signal handler.");
  		return 1;
  	}
  	if ((fdLog = open("logs/server.log", O_CREAT | O_WRONLY, FIFO_PERMS)) == -1) {
  		fprintf(stderr, "Server failed to open logfile for write : %s\n", strerror(errno));
  		return 1;
  	}
  	if ((mkfifo(mainpipename, FIFO_PERMS) == -1) && (errno != EEXIST)) {
    	perror("Server failed to create a FIFO");
    	return 1;
  	} 	
  	if ((serverFd = open(mainpipename, O_RDONLY)) == -1) {
    	perror("Server failed to open its FIFO");
    	return 1;
  	}

  	/* program dışarıdan bir sinyal tarafından sonlandırılmadıkça yaşamaya devam eder.*/
  	for ( ; ; ) {

		r_read(serverFd, &clientPid, sizeof(pid_t));
    	sprintf(clientFifoName, "%ld.fifo", (long)clientPid);
    	if ((clientFd = open(clientFifoName, O_WRONLY)) == -1) {
    		perror("Server failed to open clients FIFO");
    		return 1;
  		}

  		if (clientPid > 0 && flag == 1) {

  			childpid = fork();
  			if (childpid == -1) {
  				fprintf(stderr, "Failed to create fork.\n");
  				return 1;
  			}

  			kill(childpid, SIGUSR2);
  			if (childpid == 0) {
  				isParent = 0;
    			

    			while (flag) {

    				gettimeofday(&start, NULL);
    				createRandomMatrix(arr, size);
    				gettimeofday(&end, NULL);
    				timedif = 1000*(end.tv_sec - start.tv_sec) +
								   (end.tv_usec - start.tv_usec)/1000.0;
    				determinat = determinantOfMatrix(arr, size);
    				sprintf(str, "%d X %d matrix is generated at %ld miliseconds.\n",size, size, timedif);
    				r_write(fdLog, str, (int)strlen(str)*sizeof(char));
    				sprintf(str, "Pid of Client %d = %ld\n", numberOfClient+1, (long)clientPid);
    				r_write(fdLog, str, (int)strlen(str)*sizeof(char));
    				sprintf(str, "Determinant of matrix = %f\n\n", determinat);
    				r_write(fdLog, str, (int)strlen(str)*sizeof(char));
    				for (i = 0; i < size; i++) {
    					for (j = 0; j < size; j++)
    						r_write(clientFd, &arr[i][j], sizeof(double));
    				}
    				/* milisaniye değeri küçük girildiğinde server çok fazla matris
    				  üretmesin diye burada 3 saniye programı uyutuyorum*/
    				sleep(3);
    				sigprocmask(SIG_BLOCK, &act1.sa_mask, NULL);
    				usleep(miliseconds);
    				sigprocmask(SIG_UNBLOCK, &act1.sa_mask, NULL);
				}

  			} else {

    			clientPids[numberOfClient] = clientPid;
    			++numberOfClient;
			}
		}
		sigprocmask(SIG_BLOCK, &act1.sa_mask, NULL);
    	usleep(miliseconds);
    	sigprocmask(SIG_UNBLOCK, &act1.sa_mask, NULL);

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

