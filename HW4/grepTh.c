/*****************************************************************************
 *	HW4_161044086_Efkan_Duraklı					     *					     
 *	System Programming - grepTh using Semaphores and PIPES	             *
 *	Date: 28.04.2017						     *								 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#define PERMS (S_IRUSR | S_IWUSR)
#define WRITE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)
#define WHITESPACE ch == '\t' || ch == ' ' || ch == '\n'
#define SEM_PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)


/* thread fonksiyonuna gönderilecek parametreler için struct*/
typedef struct {
	char fileName[25];
	char string[25];
	int  totalOccurence[1];
} threadParams_t;

/* ekrana basılacak total değerler için struct*/
typedef struct {
	int numberOfFile;
	int numberOfDirectory;
	int numberOfLine;
	int totalOccurence;
}total_t;

int seacrchInFile(const char *fileName, const char *string);
void *grepWithThread(void *arg);
int searchInDirectory(const char *pathName, const char *string);
int destroynamed(char *name, sem_t *sem);
int getnamed(char *name, sem_t ** sem, int val);
int isDirectory(char *pathName);
int isRegularFile(const char *fileName);
pid_t r_wait(int *stat_loc);
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);
int r_close(int fildes);

total_t total;
int fdLog = 0;
int isParent = 1;
int totalWord = 0;
int cascadeThread = 0;
static int maxThread = 0;
static int counter = 0;
pid_t mainPid = 0;
struct timeval end;
struct timeval start;
long timedif;
static sem_t sem;


static void signal_handler(int signo) {

	if (signo == SIGUSR1)
		counter++;

	else if (signo == SIGUSR2)
		counter--;

	if (counter > maxThread)
		maxThread = counter;

	if (signo == SIGINT) {

		if (isParent) {
			/* Ctrl + c sinyali geldiği durumda child proceslerin ölmesi beklenir ve 
			   ekrana gerekli bilgiler yazılır.*/
			while(r_wait(NULL) != -1);
			printf("Total number of strings found :%d\n", totalWord);
			printf("Number of directory searched: %d\n", total.numberOfDirectory+1);
			printf("Number of files searched: %d\n", total.numberOfFile);
			printf("Number of lines searched: %d\n", total.numberOfLine);
			printf("Number of cascade threads: %d\n", cascadeThread);
			printf("Number of search threads created: %d\n", total.numberOfFile);
			printf("Maximum number of threads running concurrently: %d\n", maxThread);
			printf("Total runtime: %ld miliseconds.\n", timedif);
			printf("Exit condition: Due to SIGINT.\n");
			r_close(fdLog); 
			exit(1);
		}
	}  
}

int main(int argc, char *argv[]) {

  	char str[25];


	if (argc != 3) { /* command-line argüman sayısının kontrolü */
		fprintf(stderr, "Usage: %s string directoryname\n", argv[0]);
		return 1;
	}
	if ((fdLog = open("log.log", WRITE_FLAGS, PERMS)) == -1) {
		fprintf(stderr, "Faield to open %s. Error: %s\n", "log.log", strerror(errno));
		return 1;
	}

	mainPid = getpid();
	struct sigaction act;
  	act.sa_handler = signal_handler;
  	act.sa_flags = 0;
  	if ((sigemptyset(&act.sa_mask) == -1) || 
  	  	(sigaction(SIGUSR2, &act, NULL) == -1) ||
  	 	(sigaction(SIGUSR1, &act, NULL) == -1) ||
  	 	(sigaction(SIGINT, &act, NULL) == -1)) {
  		perror("Failed to set Signal handler.");
  		return 1;
  	}

  	/* semaphorun initialize edilmesi */
  	sem_init(&sem, 0, 1);
	
	sprintf(str, "Process ID\t\tThread ID\t\tFileName\n");
	r_write(fdLog, str, sizeof(char)*strlen(str));
	gettimeofday(&start, NULL);
	totalWord = searchInDirectory(argv[2], argv[1]);
	gettimeofday(&end, NULL);
	timedif = 1000*(end.tv_sec - start.tv_sec) +
				   (end.tv_usec - start.tv_usec)/1000.0;

	if (totalWord == -1) {
		
		fprintf(stderr, "Number of directory serched: %d\n", total.numberOfDirectory+1);
		fprintf(stderr, "Number of files searched: %d\n", total.numberOfFile);
		fprintf(stderr, "Number of lines serched: %d\n", total.numberOfLine);
		fprintf(stderr, "Number of cascade threads: %d\n", cascadeThread);
		fprintf(stderr, "Number of search threads created: %d\n", total.numberOfFile);
		fprintf(stderr, "Maximum number of threads running concurrently: %d\n", maxThread);
		fprintf(stderr, "Total runtime: %ld miliseconds.\n", timedif);
		fprintf(stderr, "Exit condition: due to an error.Error: %s\n", strerror(errno));
		r_close(fdLog);
		return 1;
	}
	sprintf(str, "%d %s were found in total.\n", totalWord, argv[1]);
	r_write(fdLog, str, sizeof(char)*strlen(str));
	printf("Total number of strings found :%d\n", totalWord);
	printf("Number of directory searched: %d\n", total.numberOfDirectory+1);
	printf("Number of files searched: %d\n", total.numberOfFile);
	printf("Number of lines searched: %d\n", total.numberOfLine);
	printf("Number of cascade threads: %d\n", cascadeThread);
	printf("Number of search threads created: %d\n", total.numberOfFile);
	printf("Maximum number of threads running concurrently: %d\n", maxThread);
	printf("Total runtime: %ld miliseconds.\n", timedif);
	printf("Exit condition: Normal.\n");

	r_close(fdLog);
	return 0;
}

int searchInDirectory(const char *pathName, const char *string) {

	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	pid_t childpid = 0;
	int *temp = NULL;
	total_t tempTotal;
	int i = 0;
	int j = 0;
	int fd[2];
	int error = 0;
	threadParams_t thread[256] ;
	pthread_t tids[256];
	pthread_t temptid = 0;
	char str[100];

	/* global total structın initialize edilmesi */
	total.totalOccurence = 0;
	total.numberOfFile = 0;
	total.numberOfDirectory = 0;
	total.numberOfLine = 0;

	/* directoryler için oluşturduğum yeni proseslerin bilgilerini toplamak için pipe*/
	if (pipe(fd) == -1) {
		perror("Failed to create the pipe");
		return -1;
	}


	if ((dirp = opendir(pathName)) == NULL) {
		fprintf(stderr, "Faield to open %s: %s\n", pathName, strerror(errno));
		return -1;
	}
	
	chdir(pathName);
	while ((direntp = readdir(dirp)) != NULL) {

		if (strcmp(direntp->d_name, "..") != 0 && strcmp(direntp->d_name, ".") != 0) {
			
			if (isRegularFile(direntp->d_name)) {

				strcpy(thread[i].fileName, direntp->d_name);
				strcpy(thread[i].string, string);
				/* her file için thread oluşturulur */
				if (error = pthread_create(&temptid, NULL, grepWithThread, thread+i)) {
					fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
					return -1;
				}
				tids[i] = temptid;
				total.numberOfFile++;
				i++;

			} else if (isDirectory(direntp->d_name)) {

				total.numberOfDirectory++;
				/* her directory için yeni bir process oluşturulur.*/
				if ((childpid = fork()) == -1) {
					perror("Failed to create fork");
					return -1;
				}
				if (childpid == 0) {
					isParent = 0;
					r_close(fd[0]);

					total.totalOccurence = searchInDirectory(direntp->d_name, string);
					r_write(fd[1], &total, sizeof(total_t));

					r_close(fd[1]);
					exit(0);
				}
			}
		}
	}
	closedir(dirp);
	/* cascade thread sayısının alınması */
	if (total.numberOfFile > cascadeThread)
		cascadeThread = total.numberOfFile;

	if (childpid > 0) {
			
			r_close(fd[1]);
			while (r_wait(NULL) != -1) {
				r_read(fd[0], &tempTotal, sizeof(total_t));
				total.totalOccurence += tempTotal.totalOccurence;
				total.numberOfFile += tempTotal.numberOfFile;
				total.numberOfDirectory += tempTotal.numberOfDirectory;
				total.numberOfLine += tempTotal.numberOfLine;
			}
			r_close(fd[0]);
		}

	j = i;
	/* threadler beklenir ve return değerleri toplanır */
	for (i = 0; i < j; i++) {

		if (pthread_equal(tids[i], pthread_self()))
			continue;
		if (error = pthread_join(tids[i], (void **)&temp)) {
			fprintf(stderr, "Failed to join thread %d %s\n", i+1, strerror(error));
			continue;
		}
		if (temp == NULL) {
			fprintf(stderr, "Thread %d failed to return status\n",i+1);
			continue;
		}
		total.totalOccurence += *temp;
	}
	dirp = NULL;
	direntp = NULL;
	temp = NULL;
	return total.totalOccurence;
}

/* file için çalışacak thread fonksiyonu */
void *grepWithThread(void *arg) {
	/* thread başladığını bildirmek için ana procese başladım sinyali gönderir.*/
	kill(mainPid, SIGUSR1);
	threadParams_t *thread  = NULL;
	thread = (threadParams_t *)arg;
	thread->totalOccurence[0] = seacrchInFile(thread->fileName, thread->string);
	/* thread öldüğünü bildirmek için ana procese bittim sinyali gönderir.*/
	kill(mainPid, SIGUSR2);
	return thread->totalOccurence;
}

int seacrchInFile(const char *fileName, const char *string) {

	FILE *filePointer;
	int i = 0;
	int rowNumber = 1;
	int columnNUmber = 1;
	int totalOccurence = 0;
	int status = 0;
	char ch;
	char str[100];

	if (NULL == (filePointer = fopen(fileName, "r"))) {
		fprintf(stderr, "File <%s> not found in directory\n", fileName);
		return -1;
	} 
	for (ch = fgetc(filePointer); ch != EOF; ch = fgetc(filePointer)) { 
		++status;
		if (ch == '\n') {   
			++rowNumber;
			columnNUmber = 1;
		}
		for (i = 0; i < strlen(string); ++i) { 
			if (ch == string[i]) {              
				ch = fgetc(filePointer);
				while(WHITESPACE)
					ch = fgetc(filePointer);
			} else break;                          
		}
		if (i == strlen(string)) { 
			sprintf(str, "%ld\t\t%ld\t\t%s:\t%s found in [%d,%d].\n", (long)getpid(), (long)pthread_self(), fileName, string, rowNumber, columnNUmber); 
			r_write(fdLog, str, sizeof(char)*strlen(str));
			++totalOccurence;
		}
		fseek(filePointer, status, SEEK_SET);   
		if (ch != '\n')
			++columnNUmber;
		}
		sem_wait(&sem);
		/* critical section */
		total.numberOfLine += rowNumber;
		sem_post(&sem);
		fclose(filePointer);
		filePointer = NULL;
		return totalOccurence;
}

/* aşağıdaki fonksiyonlar kitaptan alınmıştır*/
int isDirectory(char *pathName) {

	struct stat statbuf;

	if (stat(pathName, &statbuf) == -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}

int isRegularFile(const char *pathName) {

	struct stat statbuf;

	if (stat(pathName, &statbuf) == -1)
		return 0;
	else
		return S_ISREG(statbuf.st_mode);
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

pid_t r_wait(int *stat_loc) {
   pid_t retval;
   while (((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
   return retval;
}

int r_close(int fildes) {
   int retval;
   while (retval = close(fildes), retval == -1 && errno == EINTR) ;
   return retval;
}



