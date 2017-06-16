/***********THREAD-PER-REQUEST-SERVER****************
 *													*
 *     BIL244 System Programming Final Project    	*
 * 				server.c					        *
 * 			Student Name: Efkan Duraklı             *
 * 			Student ID  : 161044086                 *
 * 			Date        : 31/05/2017                *
 *                                                  *
 ****************************************************/

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>  
#include "methods.h"

#define MAXHOSTNAME 255
#define MAXBACKLOG 500
#define PERMS (S_IRUSR | S_IWUSR)
#define FAIL -1
#define SUCCESS 0

/* P1 ve  P2 procesleri arasndaki sahred memory bloğu */
typedef struct {
	double A[100][100];
	double b[100];
	double x[4][100];
	int row;
	int column;
}Shared1_t;

/* P2 ve  P3 procesleri arasndaki sahred memory bloğu */
typedef struct {
	double A[100][100];
	double b[100];
	double x[4][100];
}Shared2_t;

/* thread parametreleri */
typedef struct {
	int fd;
	int counter;
}Params_t;

/* global değişkenler */
typedef unsigned short portnumber_t;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t doneflag = 0; 
static int counter;

int openSocketConnection(portnumber_t portnumber);
int getConnection(int fd, char *hostn, int hostnsize);
void *client_handler(void *arg);
void *svd_method(void *arg);
void *qrfactorization_method(void *arg);
void *pseudoinverse_method(void *arg);
int r_semop(int semid, struct sembuf *sops, int nsops);
int initelement(int semid, int semnum, int semvalue);
int removesem(int semid);
void setsembuf(struct sembuf *s, int num, int op, int flg);
int detachandremove(int shmid, void *shmaddr);
void printMatrix(double a[][100], int m, int n);
void printLogFile(char *fileName, SendClient_t matrixData, int row, int column);
pid_t r_wait(int *stat_loc);
int r_close(int fildes);
static int u_ignore_sigpipe();

/* Ctl + C sinyali için sinyal handler */
static void signal_handler(int signo) {
	time_t tcurrent;
	tcurrent = time(NULL);
	doneflag = 1;
	fprintf(stderr, "Ctrl + C sinyali alındı.Server kapanıyor...\n");
	fprintf(stderr, "Termination time of server %s\n", ctime(&tcurrent));
	exit(0);
}

int main(int argc, char *argv[]) {

   	Params_t parameters[1000];
   	portnumber_t portnumber = 0;
   	pthread_t tids[1000];
   	pthread_attr_t tattr;
   	struct sigaction act;
   	char hostname[MAXHOSTNAME];
   	int listenfd = 0;
   	int i = 0;
 
   	if (argc != 2) {
      	fprintf(stderr, "Usage: %s <port #, id>\n", argv[0]);
      	return 1;   
   	} 
   	portnumber = (portnumber_t) atoi(argv[1]);
   	gethostname(hostname, MAXHOSTNAME);

   	/* logların toplanacağı directory oluşturulur. */
   	mkdir("logs", 0700);
	act.sa_handler = signal_handler;
  	act.sa_flags = 0;
  	if ((sigemptyset(&act.sa_mask) == -1) || 
  	 	(sigaction(SIGINT, &act, NULL) == -1)) {
  		perror("Failed to set Signal handler.");
  		return 1;
  	}
  	/* Clientların bağlanabileceği socket bağlantısının açılması */
   	if ((listenfd = openSocketConnection(portnumber)) == -1) {
      	perror("Failed to create listening endpoint");
      	return 1;
   	}  
   	fprintf(stderr, "[%ld]:waiting for the first connection on port %d\n",
                    (long)getpid(), (int)portnumber);

   	/* detachable thread oluşturmak için atributun ayarlanması */
   	pthread_attr_init(&tattr);
   	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

   	/* sinyal gelmediği sürece server clientlardan reuest bekler */
	while (!doneflag) {

		if ((parameters[i].fd = getConnection(listenfd, hostname, MAXHOSTNAME)) < 0) {

      	 	if (errno == EINTR)
      	 		continue;
         	perror("Failed to accept connection");
         	return 1;
         
      	}
      	/* request geldiğinde o request için bir thread oluşturulur.*/
      	parameters[i].counter = i+1;
	  	fprintf(stderr, "[%ld]:connected to client %d\n", (long)getpid(), i+1);
      	pthread_create(&tids[i], &tattr, client_handler, parameters+i);
		i++;
	}
	fprintf(stderr, "Ctrl + C sinyali alındı.Server kapanıyor...\n");
	return 0;
}

/* her bağlanan cliente hizmet eden thread */
void *client_handler(void *arg) {

   	Params_t *parameters;
   	char str[50];
   	char fileName[50];
   	int row = 0;
   	int column = 0;
   	int flag = 0;
   	int semid1 = 0;
   	int semid2 = 0;
   	int shmid1 = 0;
   	int shmid2 = 0;
   	double A[MAXSIZE][MAXSIZE];
   	double v[MAXSIZE][MAXSIZE];  
   	double b[MAXSIZE];
   	double x[MAXSIZE];
   	double w[MAXSIZE];
   	double errorNorm1 = 0;
   	double errorNorm2 = 0;
   	double errorNorm3 = 0;
   	pid_t childpid = 0;
   	long clientPid = 0;
   	int i = 0;
   	int j = 0;
   	struct sembuf semsignal[2];
	struct sembuf semwait[2];
	pthread_t tid1;
	pthread_t tid2;
	pthread_t tid3;
	pthread_t clientTid;
	Shared1_t *shared1 = NULL;
	Shared2_t *shared2 = NULL;
	SendClient_t matrixData;

	parameters = (Params_t *)arg;

   	memset (str, '\0', sizeof(str));
   	memset(&matrixData, 0, sizeof(SendClient_t));
   	recv(parameters->fd, str, sizeof(str), MSG_WAITALL);
   	sscanf(str, "%d %d %ld %ld", &row, &column, &clientTid, &clientPid);

   	/* oluştulacak olan 3 proses arasındaki senkronizasyonu sağlamak için 
   		iki tane semaphore oluşturulur.*/
	if ((semid1 = semget(IPC_PRIVATE, 1, PERMS)) == -1) 
		return NULL;
	if ((semid2 = semget(IPC_PRIVATE, 1, PERMS)) == -1) 
		return NULL;

	setsembuf(semwait, 0, -1, 0);
	setsembuf(semwait+1, 0, -1, 0);
	setsembuf(semsignal, 0, 1, 0);
	setsembuf(semsignal+1, 0, 1, 0);

	if (initelement(semid1, 0, 0) == -1) {
		removesem(semid1);
		return NULL;
	}
	if (initelement(semid2, 0, 0) == -1) {
		removesem(semid2);
		return NULL;
	}

	/* shared memorylerin oluşturulur. */
	if ((shmid1 = shmget(IPC_PRIVATE, sizeof(Shared1_t), PERMS)) == -1) 
		return NULL;
	if ((shmid2 = shmget(IPC_PRIVATE, sizeof(Shared2_t), PERMS)) == -1) 
		return NULL;

	/* üç adet process oluşturulur*/
	for (i = 0; i < 3; i++) {

   		childpid = fork();
   		if (childpid == 0)
   			break;
   		flag++;
   	}

   	/* 1. proses random matris üretir ve 1 shared memory bloğuna yazar.*/
   	if (childpid == 0 && flag == 0) {
   		
   		r_close(parameters->fd);
		if ((shared1 = (Shared1_t *)shmat(shmid1, NULL, 0)) == (void *)-1) {
			shmctl(shmid1, IPC_RMID, NULL);
			exit(FAIL);
		}
		createRandomMatrices(A, b, row, column);
		for (i = 1; i <= row; i++) {
			shared1->b[i] = b[i];
			for (j = 1; j <= column; j++) 
				shared1->A[i][j] = A[i][j];
        }
        /* 2. prosesin oluşturulan matrisleri çözebilmesi için semaforun değeri 1 artırılır.*/
		r_semop(semid1, semsignal, 1);
		exit(SUCCESS);
	}
   	else if (childpid == 0 && flag == 1) {
   		/* 2 . process 1. processin matrisleri üretmeini bekler.*/
   		r_close(parameters->fd);
   		r_semop(semid1, semwait, 1);
		if ((shared1 = (Shared1_t *)shmat(shmid1, NULL, 0)) == (void *)-1) {
			shmctl(shmid1, IPC_RMID, NULL);
			exit(FAIL);
		}

		if ((shared2 = (Shared2_t *)shmat(shmid2, NULL, 0)) == (void *)-1) {
			shmctl(shmid2, IPC_RMID, NULL);
			exit(FAIL);
		}

		for (i = 1; i <= row; i++) {
			shared2->b[i] = shared1->b[i];
			for (j = 1; j <= column; j++)
				shared2->A[i][j] = shared1->A[i][j];
		}
		shared1->row = row;
		shared1->column = column;
		/* shared memoryden alınan matrisler  3 farklı yolla çözülür.*/
		pthread_create(&tid1, NULL, svd_method, shared1);
		pthread_create(&tid2, NULL, pseudoinverse_method, shared1);
		pthread_create(&tid2, NULL, qrfactorization_method, shared1);
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);
		pthread_join(tid3, NULL);
		/* sonuçlar 2. shared memory bloğuna yazılır.*/
		for (i = 1; i <= column; i++) 
			shared2->x[1][i] = shared1->x[1][i];
		for (i = 1; i <= column; i++) 
			shared2->x[2][i] = shared1->x[2][i];
		for (i = 1; i <= column; i++) 
			shared2->x[3][i] = shared1->x[3][i];
		/* 3. proesin shared memoryden okuma yapabilmesi için semaforun değeri 1 artırılır.*/
		r_semop(semid2, semsignal+1, 1);
		removesem(semid1);
		detachandremove(shmid1, shared1);
		exit(SUCCESS);
	}
   	else if (childpid == 0 && flag == 2) {
   		/* 3. proses 2. prosesin çözümleri bulmasını bekler.*/
   		r_semop(semid2, semwait+1, 1);
		if ((shared2 = (Shared2_t *)shmat(shmid2, NULL, 0)) == (void *)-1) {
			shmctl(shmid2, IPC_RMID, NULL);
			exit(FAIL);
		}
		for (i = 1; i <= row; i++) {
			b[i] = shared2->b[i];
			x[i] = shared2->x[1][i];
			matrixData.b[i] = shared2->b[i];
			matrixData.x[1][i] = shared2->x[1][i];
			for (j = 1; j <= column; j++) { 
				A[i][j] = shared2->A[i][j];
				matrixData.A[i][j] = shared2->A[i][j];
			}
		}
		/* 2. shared memoryden datalar okunur ve hata değerleri hesaplanır.*/
		errorNorm1 = calculateErrorNorm(A, x, b, row, column);
		matrixData.error[1] = errorNorm1;
		for (i = 1; i <= row; i++) {
			x[i] = shared2->x[2][i];
			matrixData.x[2][i] = shared2->x[2][i];
		}
		errorNorm2 = calculateErrorNorm(A, x, b, row, column);
		matrixData.error[2] = errorNorm2;
		for (i = 1; i <= row; i++) {
			x[i] = shared2->x[2][i];
			matrixData.x[3][i] = shared2->x[2][i];
		}
		errorNorm3 = calculateErrorNorm(A, x, b, row, column);
		matrixData.error[3] = errorNorm3;
		/* sonuçlar clienta gönderilir.*/
		send(parameters->fd, (SendClient_t *)&matrixData, sizeof(SendClient_t), 0);
		sprintf(fileName, "logs/Server(client%d-%ld-%ld).log", parameters->counter, clientPid, clientTid);
		/* bilgiler log file a yazılır.*/
		printLogFile(fileName, matrixData, row, column);
		r_close(parameters->fd);
		removesem(semid2);
		detachandremove(shmid2, shared2);
		exit(SUCCESS);

   	}
   	else {

		r_close(parameters->fd);
   		while (r_wait(NULL) != -1);
   		pthread_exit(NULL);
   	}
}

/* Singular Value Decomposition Methoduyla çözüm üreten thread*/
void *svd_method(void *arg) {

	double A[100][100];
	double v[100][100];
	double b[100];
	double w[100];
	double x[100];
	int row = 0;
	int column = 0;
	int i = 0;
	int j = 0;
	Shared1_t *shared1;

	shared1 = (Shared1_t *)arg; 

	pthread_mutex_lock(&lock);

	/* start of critical section */
	row = shared1->row;
	column = shared1->column;

	for (i = 1; i <= row; i++) {
		b[i] = shared1->b[i];
		for (j = 1; j <= column; j++) 
			A[i][j] = shared1->A[i][j];
	}
	/* end of critical section */
	pthread_mutex_unlock(&lock);

	SingularValueDecomposition(A, row, column, w, v);
	solveWithSvd(A, w, v, row, column, b, x);

	for (i = 1; i <= row; i++)
		shared1->x[1][i] = x[i];
	pthread_exit(NULL);
}

/* QR factorization yöntemiyle çözüm üreten thread */
void *qrfactorization_method(void *arg) {

	double A[100][100];
	double b[100];
	double c[100];
	double d[100];
	int row = 0;
	int column = 0;
	int i = 0;
	int j = 0;
	Shared1_t *shared1;

	shared1 = (Shared1_t *)arg; 

	pthread_mutex_lock(&lock);

	/* start of critical section */
	row = shared1->row;
	column = shared1->column;

	for (i = 1; i <= row; i++) {
		b[i] = shared1->b[i];
		for (j = 1; j <= column; j++) 
			A[i][j] = shared1->A[i][j];
	}
	/* end of critical section */
	pthread_mutex_unlock(&lock);

	solveWithQR(A, column, c, d, b);

	for (i = 1; i <= row; i++)
		shared1->x[3][i] = b[i];

	pthread_exit(NULL);

}

void *pseudoinverse_method(void *arg) {
	
	double A[100][100];
	double b[100];
	double x[100];
	int row = 0;
	int column = 0;
	int i = 0;
	int j = 0;
	Shared1_t *shared1;

	shared1 = (Shared1_t *)arg; 


	pthread_mutex_lock(&lock);

	/* start of critical section */
	row = shared1->row;
	column = shared1->column;

	for (i = 1; i <= row; i++) {
		b[i] = shared1->b[i];
		for (j = 1; j <= column; j++) 
			A[i][j] = shared1->A[i][j];
	}
	/* end of critical section */
	pthread_mutex_unlock(&lock);


	solveWithInverseMethod(A, b, x, row, column);
 
	for (i = 1; i <= row; i++)
		shared1->x[2][i] = x[i];

	pthread_exit(NULL);
}

 /*	
 	Bu fonksiyon parametre olarak aldığı prot numarasıyla bir soket oluşturur
	ve bir integer file descriptor return eder.
*/
int openSocketConnection(portnumber_t portnumber) {

	char hostname[MAXHOSTNAME+1];
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
	memset(&server, 0, sizeof(struct sockaddr_in));
	gethostname(hostname, MAXHOSTNAME);
	hp= gethostbyname(hostname);

	if (hp == NULL) 
		return -1;

	server.sin_family= hp->h_addrtype;
	server.sin_port= htons((short)portnumber);

	if ((u_ignore_sigpipe() == -1) ||
		((sock= socket(AF_INET, SOCK_STREAM, 0)) == -1))
		return -1;

   
	if (bind(sock,(struct sockaddr *)&server,sizeof(server)) == -1) {
		r_close(sock);
		return -1;
	}

	if ( (listen(sock, MAXBACKLOG) == -1)) 
		return -1;
	
	return sock;
}

/*
	Bu fonksiyon openSoccetConnection fonksiyonunun return ettiği
	file descriptoru parametre alarak sockete bağlanan diğer process
	lerle haberleşebilcek bir file descriptor return eder.
*/
int getConnection(int fd, char *hostn, int hostnsize) { 
   int len = sizeof(struct sockaddr);
   struct sockaddr_in netclient;
   int retval;

   while (((retval =
           accept(fd, (struct sockaddr *)(&netclient), &len)) == -1) &&
          (errno == EINTR))
      ;  
   if ((retval == -1) || (hostn == NULL) || (hostnsize <= 0))
      return retval;
   return retval;
}

/* 
	Bu fonksiyon SendClient_t tipindeki structun içindeki sonuçları
	log file a basar.
*/
void printLogFile(char *fileName, SendClient_t matrixData, int row, int column) {

	FILE *filePointer;
	int i,j;

	if ((filePointer = fopen(fileName, "w")) == NULL)
		perror("Open");


	fprintf(filePointer, "Matrix A = [\n");

 	for (i = 1; i<= row; i++) {
 		fprintf(filePointer,"\t\t\t");
 		for (j = 1; j<= column; j++)
 			fprintf(filePointer, "%.4lf  ", matrixData.A[i][j]);
 		fprintf(filePointer,"\n");
 	}
 	fprintf(filePointer,"\t\t]\n\n");
 	fprintf(filePointer, "Vector b = [  ");
 	for (i = 1; i <= row; i++)
 		fprintf(filePointer, "%.4lf  ", matrixData.b[i]);
 	fprintf(filePointer, "]\n\n");
 	fprintf(filePointer, "Vector x1 = [  ");
  	for (i = 1; i <= column; i++)
 		fprintf(filePointer, "%.4lf  ", matrixData.x[1][i]);
 	fprintf(filePointer, "]\n\n");
 	fprintf(filePointer, "Vector x2 = [  ");
  	for (i = 1; i <= column; i++)
 		fprintf(filePointer, "%.4lf  ", matrixData.x[2][i]);
 	fprintf(filePointer, "]\n\n");
 	fprintf(filePointer, "Vector x3 = [  ");
  	for (i = 1; i <= column; i++)
 		fprintf(filePointer, "%.4lf  ", matrixData.x[3][i]);
 	fprintf(filePointer, "]\n\n");
 	fprintf(filePointer, "Error Norm 1 = %lf\n", matrixData.error[1]);
 	fprintf(filePointer, "Error Norm 2 = %lf\n", matrixData.error[2]);
 	fprintf(filePointer, "Error Norm 3 = %lf\n", matrixData.error[3]);
 	fclose(filePointer);
}


/* aşağıdaki fonksiyonlar kitaptan alınıştır */
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

int detachandremove(int shmid, void *shmaddr) {
   int error = 0; 

   if (shmdt(shmaddr) == -1)
      error = errno;
   if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
      error = errno;
   if (!error)
      return 0;
   errno = error;
   return -1;
}

static int u_ignore_sigpipe() {
   struct sigaction act;

   if (sigaction(SIGPIPE, (struct sigaction *)NULL, &act) == -1)
      return -1;
   if (act.sa_handler == SIG_DFL) {
      act.sa_handler = SIG_IGN;
      if (sigaction(SIGPIPE, &act, (struct sigaction *)NULL) == -1)
         return -1;
   }   
   return 0;
}

int r_close(int fildes) {
   int retval;
   while (retval = close(fildes), retval == -1 && errno == EINTR) ;
   return retval;
}

pid_t r_wait(int *stat_loc) {
   pid_t retval;
   while (((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
   return retval;
}






