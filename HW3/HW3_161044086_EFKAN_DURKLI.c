/*-------------------------------------------------------------------------*
 * HW2_161044086_EFKAN_DURKLI.c                                            *
 *-------------------------------------------------------------------------*
 * Written by Efkan DURAKLI on March 9, 2017                               *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * Bu program terminalden command-line argüman olarak verilen path içinde  *
 * gene terminalden girilen kelimenin whitespace karakterleri dikkate      *
 * almadan kelimenin ilk harfinin geçtiği satır ve sütünu,toplam geçme     * 
 * sayısını ve geçtiği dosya ismini bir log dosyası oluşturarak o dosyaya  *
 * yazar.Toplam geçme sayısını hesaplamak için proseslerin haberleşmesi    *
 * fifoyla sağlanır.                                                       *
 *						                           *											   *
 *-------------------------------------------------------------------------*
 */
 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#define WHITESPACE ch == '\t' || ch == ' ' || ch == '\n'
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define BLKSIZE 512
#define MAX_SIZE 100
#define EXIT_FILE 1
#define EXIT_DIRECTORY 2
#define EXIT_FAIL -5

/*
 * Bu fonksiyon kitaptan alınmıştır.
 * Parametre olarak path isminin directory olup
 * olmadığını kontrol eder.
 * Parametreleri
 *		pathName : kontrol edilcek pathin adı
 * Return değeri : Eğer path directory ise 0 dan farklı bir değer,
 *					değilse 0 return eder.
 */
int isDirectory(char *pathName);
/*
 * Parametre olarak path isminin regular file olup
 * olmadığını kontrol eder.
 * Parametreleri
 *		pathName : kontrol edilcek pathin adı
 * Return değeri : Eğer path directory ise 0 dan farklı bir değer,
 *					değilse 0 return eder.
 */
int isRegularFile(const char *fileName);
/*bu fonksiyon kitaptan alınmıştır.*/
pid_t r_wait(int *stat_loc);
/*bu fonksiyon kitaptan alınmıştır.*/
ssize_t r_write(int fd, void *buf, size_t size);
/*bu fonksiyon kitaptan alınmıştır.*/
ssize_t r_read(int fd, void *buf, size_t size);
/*
 * Bu fonksiyon kendisine parametre olarak gelen fileName isimli dosyayı
 * açarak bu dosyanın içinde fonksiyona parametre olarak gelen stringi
 * arar ve kelimenin ilk karakterinin dosya içerisindede bulunduğu konumu
 * parametre olarak gelen logFile isimli dosyaya yazar.
 * Parametreleri: 
 *		fileName: kelimenin aranacağı dosyanın ismi
 *		string  : dosya içerisinde aranacak kelime
 *      logFile : outputun yazılacağı log dosyasının ismi
 * Return değeri: kelimenin dosyada kaç kez geçtiğini return eder.
 *                 hata durumlarında -1 return eder.
 */
int searchInFile(const char *fileName, const char *string, int fdLog); 
/*
 * Bu fonksiyon kendisine parametre olarak gelen pathin içinde bulunan
 * bütün regular dosyalarda ve directorylerde parametre olarak verilen stringi 
 * arar. Gördüğü her dosya ve klasör için ayrı prosesler oluşturur.
 * Eğer yeni bir directory bulduğunda recursive olarak devam eder.
 * Kelimenin pathdeki bütün dosyalarda geçme sayısı pipe ve fifo kullanılarak
 * bulunur.
 * Parametreleri:
 *	    pathName: stringin aranacağı pathin adı.
 *          string  : directoryler içerisinde aranacak string.
 *          logFile : Outputun yazılacağı log dosyasının adı.
 * Return değeri: Bütün dosya ve klasörlerde kelimenin toplam bulunma sayısını return eder.
 *                 hata durumlarında -1 return eder.
 */
int searchInDirectory(const char *pathName, const char *string, int fdLog);

int main(int argc, char const *argv[]) {

	int totalOccurence = 0;
	int fdLog;
	char tempString[MAX_SIZE];

	if (argc != 3) { /* command-line argüman sayısının kontrolü */
		fprintf(stderr, "Usage: %s string directoryname\n", argv[0]);
		return 1;
	}
	if ((fdLog = open("log.log", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		fprintf(stderr, "Faield to open %s. Error: %s\n", "log.log", strerror(errno));
		return 1;
	}
	totalOccurence = searchInDirectory(argv[2], argv[1], fdLog);
	sprintf(tempString, "%d %s were found in total.\n", totalOccurence, argv[1]);
	/* toplam kelime sayısının log dosyasına yazılması */
	r_write(fdLog, tempString, strlen(tempString));
	close(fdLog);
	return 0;
}

int searchInDirectory(const char *pathName, const char *string, int fdLog) {

	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	pid_t childpid = 0;
	char direntName[PATH_MAX];
	char cwd[PATH_MAX];
	int totalOccurence = 0;
	int childStatus = 0;
	int fileFlag = 0;
	int dirFlag = 0;
	int fdPipe[2];
	int fdFifo[2];
	int temp = 0;
	int whodead = 0;

	if ((dirp = opendir(pathName)) == NULL) {
		fprintf(stderr, "Failed to open %s. Error: %s\n", pathName, strerror(errno));
		return -1;
	}
	chdir(pathName);
	while ((direntp = readdir(dirp)) != NULL) {
    		if(strcmp(direntp->d_name, "..") != 0 && strcmp(direntp->d_name, ".") != 0) {
      			if (isRegularFile(direntp->d_name)) 
      				fileFlag = 1;
    			if (isDirectory(direntp->d_name)) 
        			dirFlag = 1;
      		}
  	}
  	/* pathin içinde file varsa pipe oluşturulur */
  	if (fileFlag) {
  		if (pipe(fdPipe) == -1) {
  			fprintf(stderr, "Failed to create pipe.Error: %s\n", strerror(errno));
			return -1;
  		}
  	}
  	/* pathin içinde directory varsa fifo oluşturulur */
  	if (dirFlag) {
  		if (mkfifo("efkan.fifo", FIFO_PERMS) == -1) {
			if (errno != EEXIST) {
				fprintf(stderr, "[%ld]:failed to create named pipe %s.Error: %s\n", 
					   (long)getpid(), "efkan.fifo", strerror(errno));
				return -1;
			}
		}
		/* fifonun read kapısı nonblock modda açılır */
		while(((fdFifo[0] = open("efkan.fifo", O_RDONLY | O_NONBLOCK)) == -1) && (errno = EINTR)) ;
		if (fdFifo[0] == -1) {
			fprintf(stderr, "[%ld]:failed to open named pipe %s.Error: %s\n",
				   (long)getpid(), "efkan.fifo", strerror(errno));
			return -1;
		}
  	}
  	/* pathin içindeki  her fiel va directory için yeni proses oluşturulur*/
  	rewinddir(dirp);
  	while ((direntp = readdir(dirp)) != NULL) {
		if (strcmp(direntp->d_name, "..") != 0 && strcmp(direntp->d_name, ".")) {
			if (isRegularFile(direntp->d_name) || isDirectory(direntp->d_name)) {
				childpid = fork(); 
				if (childpid <= 0) {
					strcpy(direntName, direntp->d_name);
					closedir(dirp); 
					break;
				}
			}
		}
	}
	if (childpid > 0) {
		close(fdPipe[1]);
		while(r_wait(&childStatus) != -1) {
			whodead = WEXITSTATUS(childStatus);
			/* file ise pipedan okuma yapılır */
			if (whodead == EXIT_FILE) {
				while(r_read(fdPipe[0], &temp, sizeof(int))) 
					totalOccurence += temp;
			} 
			/* directory ise fifodan okuma yapılır */
			if (whodead == EXIT_DIRECTORY) {
				while(r_read(fdFifo[0], &temp, sizeof(int)))
					totalOccurence += temp;
				close(fdFifo[0]);
			}
		}
	}
	if (childpid == 0) {
		close(fdPipe[0]);
		if (isRegularFile(direntName)) {
			/* file içindeki toplam kelime sayısı pipea yazılır */
			temp = searchInFile(direntName, string, fdLog);
			r_write(fdPipe[1], &temp, sizeof(int));
			close(fdPipe[1]);
			exit(EXIT_FILE);
		} else if (isDirectory(direntName)) {
			while (((fdFifo[1] = open("efkan.fifo", O_WRONLY | O_NONBLOCK)) == -1) && (errno == EINTR)) ;
			if (fdFifo[1] == -1) {
				fprintf(stderr, "[%ld]:Failed to open named pipe %s for write.Error: %s\n", 
					   (long)getpid(), "efkan.fifo", strerror(errno));
				exit(EXIT_FAIL);
			}
			/* klasör içinde geçen toplam kelime sayısı fifoya yazılır */
			temp = searchInDirectory(direntName, string, fdLog);
			r_write(fdFifo[1], &temp, sizeof(int));
			close(fdFifo[1]);
			unlink("efkan.fifo");
			exit(EXIT_DIRECTORY);
		}
		closedir(dirp);
		dirp = NULL;
		direntp = NULL;
		
	} 
	return totalOccurence;
}

int searchInFile(const char *fileName, const char *string, int fdLog) {

	FILE *textFile;
	int i = 0;
	int rowNumber = 1;
	int columnNumber = 1;
	int totalOccurence = 0;
	int counter = 0;
	char tempString[50];
	char ch;

	if ((textFile = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "Failed to open %s. Error: %s\n", fileName, strerror(errno));
		return -1;
	}
	for (ch = fgetc(textFile); ch != EOF; ch = fgetc(textFile)) {
		counter++;
		if (ch == '\n') {
			++rowNumber;
			columnNumber = 1;
		}
		for (i = 0; i < strlen(string); ++i) {
			if (ch == string[i]) {
				ch = fgetc(textFile);
				while (WHITESPACE)
					ch = fgetc(textFile);
			} else break;
		}
		if (i == strlen(string)) {
			sprintf(tempString, "%s: [%d,%d] %s first character is found.\n", fileName, rowNumber, columnNumber, string);
			write(fdLog, tempString, strlen(tempString));
			++totalOccurence;
		}
		fseek(textFile, counter, SEEK_SET);
		if (ch != '\n')
			++columnNumber;
	}
	fclose(textFile);
	return totalOccurence;
}


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
