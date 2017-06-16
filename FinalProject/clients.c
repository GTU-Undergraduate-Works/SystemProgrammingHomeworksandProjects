/***********CLIENTS PROGRAM**************************
 *                                                  *
 *     BIL244 System Programming Final Project      *
 *             server2.c                            *
 *          Student Name: Efkan Duraklı             *
 *          Student ID  : 161044086                 *
 *          Date        : 31/05/2017                *
 *                                                  *
 ****************************************************/

#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <semaphore.h>
#include <errno.h>
#include "methods.h"

#define MAXHOSTNAME 255
#define MILLION 1000000L
typedef unsigned short portnumber_t;

void *request_thread(void *arg);
int connectSocket(portnumber_t portnumber, char *hostname);
void printLogFile(char *fileName, SendClient_t matrixData, int row, int column);
void printMatrix(double a[][100], int m, int n);
long double getdifMic(struct timeval *start, struct timeval *end);
pid_t r_wait(int *stat_loc);
int r_close(int fildes);

/* Thread parametreleri */
typedef struct {
   portnumber_t portnumber;
   char hostname[MAXHOSTNAME];
   int row;
   int column;
   int counter;
} threadParams_t;

/* global değişkenler */
static sem_t sem;
int connectionTime[1000];



int main(int argc, char const *argv[]) {

	int numberOfThreads = 0;
	int i = 0;
   int error = 0;
   int row = 0;
   int column = 0;
   char fileName[50];
   pthread_t tids[1000];
   char hostname[MAXHOSTNAME];
   double average;
   double stdeviation;
   struct sigaction act;
   FILE *filePointer;
   threadParams_t thread[1000];
   portnumber_t portnumber;



   if (argc != 5) {
      fprintf(stderr, "Usage: %s <port #, id>  <#of columns of A, m> <#of rows of A, p> <#of clients, q>\n", argv[0]);
      return 1;
   }
   portnumber = (portnumber_t)atoi(argv[1]);
   row = atoi(argv[2]);
   column = atoi(argv[3]);

   if (row < column) {
   	fprintf(stderr, "Row dimension must be greater than or equal to column dimension.\n");
   	return 1;
   } 
   if (row > 99 || column > 99) {
      fprintf(stderr, "Row and column can be max 99\n");
      return 1;
   }
   numberOfThreads = atoi(argv[4]);
   /* makine isminin alınması */
   gethostname(hostname, MAXHOSTNAME);
   /* semaforun initialize edilmesi */
   sem_init(&sem, 0, 1);
   /* client threadlar oluşturulur.*/
	for (i = 0; i < numberOfThreads; i++) {
      thread[i].counter = i+1;
      thread[i].portnumber = portnumber;
      strcpy(thread[i].hostname,hostname);
      thread[i].row = row;
      thread[i].column = column;
      pthread_create(&tids[i], NULL, request_thread, thread+i);
   }

   /* threadlerin beklenmesi */
   for (i = 0; i < numberOfThreads; i++) {
      if (pthread_equal(tids[i], pthread_self()))
         continue;
      if (error = pthread_join(tids[i], NULL)) 
         continue;
   }

   /*client programının log dosyasının açılması */
 	sprintf(fileName, "logs/clients-%ld.log", (long)getpid());
   if ((filePointer = fopen(fileName, "w")) == NULL) {
      fprintf(stderr, "Clients failed to open log file\n");
      return 1;
   }

   average = averageConnectionTime(connectionTime, i);
   stdeviation = calculateStandardDeviation(connectionTime, average, i);
   fprintf(filePointer, "Average Connection Time = %.4lf microseconds\n", average);
   fprintf(filePointer, "Standard devition of overall run = %.4lf microseconds\n", stdeviation);
   fclose(filePointer);
   return 0;
}

/* serverdan matris üretmesi için istekte bulunan thread */
void *request_thread(void *arg) {

   int communfd;
   int i,j;
   int t;
   threadParams_t *thread = NULL;
   thread = (threadParams_t *)arg;
   char str[50];
   char fileName[100];
   FILE *filePointer = NULL;
   SendClient_t matrixData;
	struct timeval end;
	struct timeval start;
	long timedif;

   memset (str, '\0', sizeof(str));
   memset(&matrixData, 0, sizeof(SendClient_t));

   while (sem_wait(&sem) == -1)
	if(errno != EINTR) {
		fprintf(stderr, "Thread failed to lock semaphore\n");
		return NULL;
	}
   /* critical section */
   /* sockete bağlanılır.*/
   if ((communfd = connectSocket(thread->portnumber, thread->hostname)) == -1) {
      fprintf(stderr, "Server bulunamadı.\n");
      /* server kapalıysa program SIGINT sinyali ile kapanır.*/
      kill(getpid(), SIGINT);
      return NULL;
   }
   fprintf(stderr, "Client %d [%ld]:connected to Server.\n", thread->counter, (long)getpid());

	if (sem_post(&sem) == -1) {
		fprintf(stderr, "Thread failed to unlock semaphore\n");
		return NULL;
	}
   /* clientın bağlanma zamanının alınması */
	gettimeofday(&start, NULL);
	/* servera bilgilerin gönderilemesi */
   sprintf(str, "%d %d %ld %ld", thread->row, thread->column, (long)pthread_self(), (long)getpid());
   send(communfd, str, sizeof(str), 0);
   /* serverdan matris ilgilerinin alınması */
   recv(communfd, (SendClient_t *)&matrixData, sizeof(SendClient_t), MSG_WAITALL);
   r_close(communfd);
   sprintf(fileName, "logs/client%d-%ld-%ld.log", thread->counter, (long)getpid(), (long)pthread_self());
   /* serverdan alınan bilgilerin log file a yazılması */
   printLogFile(fileName, matrixData, thread->row, thread->column);
   /* clientın bitiş zamanının alınması*/
   gettimeofday(&end, NULL);
   /* clientın çalışma süresi mikrosaniye olarak bulunur.*/
   timedif = getdifMic(&start, &end);
   connectionTime[thread->counter] = timedif;
   pthread_exit(NULL);
}

/* 
   Bu fonkiyon parametre olarak portnumarası ve hostname alır ve
   port nuarası belirtilen sıkete bağlanır.Başarılı olursa serverla 
   haberleşebileeği bir file descriptor return eder.Hata durumalrında -1 
   return eder
*/
int connectSocket(portnumber_t portnumber, char *hostname) {

   struct sockaddr_in client;
   struct hostent *hp;
   int sock;

   /* makinenin isminin alınması */
   gethostname(hostname, MAXHOSTNAME);
   hp= gethostbyname(hostname);

   if (hp == NULL)
      return -1;
   
   memset(&client,0,sizeof(client));
   memcpy((char *)&client.sin_addr,hp->h_addr,hp->h_length); /* set address */
   client.sin_family= hp->h_addrtype;
   client.sin_port= htons((short)portnumber);

   if ((sock= socket(hp->h_addrtype,SOCK_STREAM,0)) < 0)
      return -1;
   if (connect(sock,(struct sockaddr *)&client,sizeof client) < 0) { /* connect */
      r_close(sock);
      return -1;
   } 
   return sock;
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

/* ikizaman arasındaki farkı mikrosaniye resolutionunda hesaplar.*/
long double getdifMic(struct timeval *start, struct timeval *end) {
    return MILLION * (end->tv_sec - start->tv_sec) + end->tv_usec - start->tv_usec;
}

/* aşağıdaki fonksiyonlar kitaptan alınmıştır.*/
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




