/**************************************************************************************
 *	HW4_161044086_Efkan_Duraklı						      *				          
 *	System Programming - grepSh using Semaphores,Shared Memory and 	Message queue *
 *	Date: 12.05.2017							      *							      *
 **************************************************************************************/

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
#include <sys/sem.h>
#include <sys/shm.h> 
#include <sys/msg.h>

#define WHITESPACE ch == '\t' || ch == ' ' || ch == '\n'
#define WRITE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)
#define PERMS (S_IRUSR | S_IWUSR)
#define MSGSIZE 50

/* thread parametreleri için struct */
typedef struct {
	char fileName[25];
	char string[25];
	int  totalOccurence[1];
} threadParams_t;

/* total line ve kelime sayısı için struct */
typedef struct {
	int numberOfLine;
	int totalOccurence;
}total_t;

/* message que ya yazılacak bilgiler için struct */
typedef struct {
	long mtype;
	char mtext[MSGSIZE];
} mymsg_t;


int searchInDirectory(const char *pathName, const char *string);
int seacrchInFile(const char *fileName, const char *string);
int searchInDirectoryRecusive(const char *pathName, const char *string);
void *grepWithThread(void *arg);
int isDirectory(char *pathName);
int isRegularFile(const char *fileName);
pid_t r_wait(int *stat_loc);
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);
int r_close(int fildes);
int r_semop(int semid, struct sembuf *sops, int nsops);
int initelement(int semid, int semnum, int semvalue);
int removesem(int semid);
void setsembuf(struct sembuf *s, int num, int op, int flg);
int detachandremove(int shmid, void *shmaddr);



int fdLog = 0;
total_t *total;
struct sembuf semsignal[1];
struct sembuf semwait[1];
static int queueid;
static int semid = 0;
static int maxThread = 0;
pid_t mainPid = 0;
static volatile sig_atomic_t doneflag = 0;
FILE *tempFile1 = NULL;
FILE *tempFile2 = NULL;

/* SIGINT, SIGUSR1, SIGUSR2 için signal handler*/
static void signal_handler(int signo) {

	static int counter = 0;

	if (signo == SIGINT)
		doneflag = 1;
	else {
		if (signo == SIGUSR1)
			counter++;
		else
			counter--;

		if (counter > maxThread)
			maxThread = counter;
	}
}

int main(int argc, char *argv[]) {

	int error = 0;
	struct sigaction act;
	struct timeval end;
	struct timeval start;
	long timedif;
	char str[100];


	if (argc != 3) { /* command-line argüman sayısının kontrolü */
		fprintf(stderr, "Usage: %s string directoryname\n", argv[0]);
		return 1;
	}
	act.sa_handler = signal_handler;
  	act.sa_flags = 0;
  	if ((sigemptyset(&act.sa_mask) == -1) || 
  	  	(sigaction(SIGUSR2, &act, NULL) == -1) ||
  	 	(sigaction(SIGUSR1, &act, NULL) == -1) ||
  	 	(sigaction(SIGINT, &act, NULL) == -1)) {
  		perror("Failed to set Signal handler.");
  		return 1;
  	}

	if ((fdLog = open("log.log", WRITE_FLAGS, PERMS)) == -1) {
		fprintf(stderr, "Faield to open %s. Error: %s\n", "log.log", strerror(errno));
		return 1;
	}

	sprintf(str, "Process ID\t\tThread ID\t\tFileName\n");
	r_write(fdLog, str, sizeof(char)*strlen(str));

	gettimeofday(&start, NULL);
	error = searchInDirectory(argv[2], argv[1]);
	gettimeofday(&end, NULL);
	timedif = 1000*(end.tv_sec - start.tv_sec) +
				   (end.tv_usec - start.tv_usec)/1000.0;

	printf("Total runtime: %ld miliseconds.\n", timedif);

	if (doneflag)
		printf("Exit condition: Due to SIGINT.\n");
	else  {

		if (error == 0)
			printf("Exit condition: Normal.\n");
		else
			printf("Exit condition: Due to error.Error : %s\n", strerror(error));
	}
	r_close(fdLog);
	return 0;
}

/* 
	Bu fonksiyon recursive fonksiyonun çağrıldığı yardımcı fonksiyondur.
	Normal durumlarda 0, hata durumlarında -1 return eder.
*/
int searchInDirectory(const char *pathName, const char *string) {

	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	mymsg_t mymessage;
	int cascades[256];
	int error = 0;
	int i = 0;
	int x = 0;
	int temp = 0;
	int temp1,temp2;
	int totalFile = 0;
	int totalDirectory = 0;
	int totalOccurence = 0;
	int totalLine = 0;
	char str[100];

	mainPid = getpid();
	/* semafor seti oluşturulur */
	if ((semid = semget(IPC_PRIVATE, 1, PERMS)) == -1) {
		perror("Failed to create a private semaphore");
		return errno;
	}
	/* semaforlar set edilir. */
	setsembuf(semwait, 0, -1, 0);
	setsembuf(semsignal, 0, 1, 0);
	/* semofor seti initialize edilir.*/
	if (initelement(semid, 0, 1) == -1) {
		perror("Failed to initialize semaphore element to 1");
		if (removesem(semid) == -1)
			perror("Failed to remove failed semaphore");
		return errno;
	}

	tempFile1 = tmpfile();
	tempFile2 = tmpfile();

	/* message queue oluşturulur.*/
	if ((queueid = msgget(98765, PERMS | IPC_CREAT)) == -1) {
		fprintf(stderr, "Failed to create message queue\n");
		return errno;
	}

	/* directoryinin içinde string recursive fonksiyonla aranır*/
	if ((searchInDirectoryRecusive(pathName, string) == -1)) {
		fprintf(stderr, "Failed to search directory\n");
		return errno;
	}

	fseek(tempFile1, 0, SEEK_SET);
	while (fread(&temp,sizeof(int), 1, tempFile1))
		totalDirectory += temp;

	fseek(tempFile2, 0, SEEK_SET);
	while (fread(&temp, sizeof(int), 1, tempFile2)) {
		
		totalFile += temp;
		cascades[i] = temp;
		i++;
	}
	for (i = 0; i < totalDirectory+1; i++) {

		msgrcv(queueid, &mymessage, MSGSIZE, 1, 0);
		sscanf(mymessage.mtext, "%d %d", &temp1, &temp2);
		totalLine += temp1;
		totalOccurence += temp2; 
	}
	
	msgctl(queueid, IPC_RMID, NULL);

	if ((error = removesem(semid)) == -1) {
		fprintf(stderr,"Failed to clean up: %s\n", strerror(error)); 
		return errno;
	}

	printf("Total number of strings found :%d\n", totalOccurence);
	printf("Number of directory searched: %d\n", totalDirectory+1);
	printf("Number of files searched: %d\n", totalFile);
	printf("Number of lines searched: %d\n", totalLine);
	printf("Number of cascade threads: ");
	for (i = 0; i < totalDirectory+1; i++) {
		if (cascades[i] > 0)
			printf("%d ", cascades[i]);
	}
	printf("\nNumber of search threads created: %d\n", totalFile);
	printf("Number of shared memory created: %d. Their's size: ", totalDirectory+1);
	for (i = 0; i < totalDirectory+1; i++) {
		if (cascades[i] > 0)
			printf("%d ", 8*cascades[i]);
	}
	printf("\nMaximum number of threads running concurrently: %d\n", maxThread);

	sprintf(str, "%d %s were found in total.\n", totalOccurence, string);
	r_write(fdLog, str, sizeof(char)*strlen(str));

	fclose(tempFile1);
	fclose(tempFile2);
	return 0;
}

/* 
   Bu fonksiyon kendisine parametre gelen pathin içindeki doyalarda gene parametre
   olarak gelen string kelimesini recursive olarak arar ve fileların bilgileri log fila
   yazılır.Hata durumlarında -1 normal durumlarda 0 return eder.

*/
int searchInDirectoryRecusive(const char *pathName, const char *string) {

	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	int i = 0, j = 0;
	int shmid = 0;
	int error = 0;
	int *temp = NULL;
	int numberOfFile = 0;
	int numberOfDirectory = 0;
	threadParams_t thread[256] ;
	pid_t childpid = 0;
	pthread_t tids[256];
	pthread_t temptid = 0;
	mymsg_t mymessage;

	if ((shmid = shmget(IPC_PRIVATE, sizeof(total_t), PERMS)) == -1) {
		perror("Failed to create shared memory segment");
		return -1;
	}

	if ((total = (total_t *)shmat(shmid, NULL, 0)) == (void *)-1) {
		perror("Failed to attach shared memory segment");
		if (shmctl(shmid, IPC_RMID, NULL) == -1)
			perror("Failed to remove memory segment");
			return -1;
	}
	total->totalOccurence = 0;
	total->numberOfLine = 0;

	if ((dirp = opendir(pathName)) == NULL) {
		fprintf(stderr, "Faield to open %s: %s\n", pathName, strerror(errno));
		return -1;
	}
	chdir(pathName);
	while ((direntp = readdir(dirp)) != NULL) {

		if (strcmp(direntp->d_name,"..") != 0 && strcmp(direntp->d_name, ".") != 0) {

			if (isRegularFile(direntp->d_name)) {

				
				strcpy(thread[i].fileName, direntp->d_name);
				strcpy(thread[i].string, string);

				if (error = pthread_create(&temptid, NULL, grepWithThread, thread+i)) {
					fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
					return -1;
				}
				tids[i] = temptid;
				numberOfFile++;
				i++;

			}  else if (isDirectory(direntp->d_name)) {

				numberOfDirectory++;
				if ((childpid = fork()) == -1) {
					fprintf(stderr, "Failed to create fork.\n");
					return -1;
				}

				if (childpid == 0) {
					searchInDirectoryRecusive(direntp->d_name, string);
					exit(0);
				}
			}
		}
	}
	closedir(dirp);

	if (childpid > 0)
		while (r_wait(NULL) != -1);

	j = i;
	total->totalOccurence = 0;
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
		
		total->totalOccurence+= *temp;
	}

	fwrite(&numberOfDirectory, sizeof(int), 1, tempFile1);
	fwrite(&numberOfFile, sizeof(int), 1, tempFile2);
	sprintf(mymessage.mtext, "%d %d", total->numberOfLine, total->totalOccurence);
	mymessage.mtype = 1;
	if (msgsnd(queueid, &mymessage, strlen(mymessage.mtext)+1, 0) < 0) {
		fprintf(stderr, "Failed to send message to message queue : %s\n", strerror(errno));
		return -1;
	}

	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}

/* thread fonksiyonu */
void *grepWithThread(void *arg) {

    /* thread çalışmaya başladığında ana prosese başladım sinyali gönderilir.*/
	kill(mainPid, SIGUSR1);
	threadParams_t *thread  = NULL;
	thread = (threadParams_t *)arg;
	thread->totalOccurence[0] = seacrchInFile(thread->fileName, thread->string);
	/* thread biterken biteceğini ana prosese bildirrir.*/
	kill(mainPid, SIGUSR2);
	return thread->totalOccurence;
}

/* 
   Bu fonksiyon kendisine parametre olarak gelen filename ismindeki
   file ı açarak fonksiyona parametre olarak gelen stringi arayarak
   fileın adını ve stringin ilk harfinin bulunduğu konumu log dosyasına yazar
   ve toplam kelime sayısını return eder.Hata durmlarında -1 return eder.
 */
int seacrchInFile(const char *fileName, const char *string) {

	FILE *filePointer;
	int i = 0;
	int rowNumber = 1;
	int columnNUmber = 1;
	int totalOccurence = 0;
	int status = 0;
	int error = 0;
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

		if (((error = r_semop(semid, semwait, 1)) == -1)) {
			fprintf(stderr, "Failed to lock semaphore: %s\n", strerror(error));
			return -1;
		}
		total->numberOfLine += rowNumber;

		if (((error = r_semop(semid, semsignal, 1)) == -1)) 
			fprintf(stderr, "Failed to lock semaphore: %s\n", strerror(error));

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

int r_semop(int semid, struct sembuf *sops, int nsops) {
   while (semop(semid, sops, nsops) == -1) 
      if (errno != EINTR) 
         return -1;
   return 0; 
}

int initelement(int semid, int semnum, int semvalue) {
   union semun {
      int val;
      struct semid_ds *buf;
      unsigned short *array;
   } arg;
   arg.val = semvalue;
   return semctl(semid, semnum, SETVAL, arg);
}


void setsembuf(struct sembuf *s, int num, int op, int flg) {
   s->sem_num = (short)num;
   s->sem_op = (short)op;
   s->sem_flg = (short)flg;
   return;
}

int removesem(int semid) {
   return semctl(semid, 0, IPC_RMID);
}







