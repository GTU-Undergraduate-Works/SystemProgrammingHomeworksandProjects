/*********************************************************************************
 *       HW2_161044086_EFKAN_DURAKLI.c                                           *
 *       System Programming Homework1                                            *
 *		 Author: Efkan Duraklı                                                   *
 *       Date: 09.03.2017                                                        *
 *-------------------------------------------------------------------------------*
 * Bu program verilen path içerisinde bulunan txt dosyalarında verilen kelimeyi  *
 * arar.Kelime aranırken kelimeyi bölen whitespace karakterler igonere edilir.   *                                                     *
 * Kelimenin ilk harfinin dosyada bulunduğu satır,sütun ve kelimenin dosyada     *												 *
 * toplam bulunma sayısı ekrana yazdırılır.                                      *
 *********************************************************************************/
 
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

/* whitespace karakterler için makro */
#define WHITESPACE ch == '\t' || ch == ' ' || ch == '\n'


int isDirectory(char *pathName);


int isTextFile(const char *fileName); 


int searchInFile(const char *fileName, const char *string, FILE *logFile); 


int searchInDirectory(const char *pathName, const char *string, FILE *logFile);

int main(int argc, char const *argv[]) {

	int totalOccurence = 0;
	FILE *logFile;

	if (argc != 3) { /* command-line argüman sayısının kontrolü*/
		fprintf(stderr, "Usage: %s string directoryname\n", argv[0]);
		return 1;
	}
	if ((logFile = fopen("log.log", "a")) == NULL) {
		fprintf(stderr, "Faield to open %s. Error: %s\n", "log.log", strerror(errno));
		return 1;
	}
	totalOccurence = searchInDirectory(argv[2], argv[1], logFile);
	fprintf(logFile, "%d %s were found in total.\n", totalOccurence, argv[1]);
	fclose(logFile);
	return 0;
}

/*
 * Bu fonksiyon kendisine parametre olarak gelen pathin içinde bulunan
 * bütün txt dosyalarında ve directorylerde parametre olarak verilen stringi 
 * arar.Her dosya ve klasör için ayrı prosesler oluşturur.Yeni bir directory
 * gördüğünde recursive olarak devam eder.
 * Parametreleri:
 *			pathName: stringin aranacağı pathin adı.
 *          string  : directoryler içerisinde aranacak string.
 *          logFile : Outputun yazılacağı log dosyasının adı.
 * Return değeri: Bütün dosya ve klasörlerde kelimenin toplam bulunma sayısını return eder.
 *                 hata durumlarında -1 return eder.
 */
int searchInDirectory(const char *pathName, const char *string, FILE *logFile) {

	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	pid_t childpid = 0;
	pid_t waitedChild = 0;
	char cwd[PATH_MAX];
	char direntName[PATH_MAX];
	int childStatus = 0;
	int totalOccurence = 0;
	int waitReturn = 0;

	if ((dirp = opendir(pathName)) == NULL) {
		fprintf(stderr, "Failed to open %s. Error: %s\n", pathName, strerror(errno));
		return -1;
	}
	/* pathin içine girilir */
	chdir(pathName);
	while ((direntp = readdir(dirp)) != NULL) {
		if (strcmp(direntp->d_name, "..") != 0 && strcmp(direntp->d_name, ".")) {
			if (isTextFile(direntp->d_name) || isDirectory(direntp->d_name)) {
				childpid = fork(); 
/* child proseslerin tekrardan döngüye girip proses oluşturmaları engellenir*/
				if (childpid <= 0) {
					strcpy(direntName, direntp->d_name);
					closedir(dirp); 
					break;
				}
			}
		}
	}
	/* toplam bulunma sayısının child proseslerden alınması */
	if (childpid > 0) { 
		while ((waitReturn = wait(&childStatus)) != -1) 
			totalOccurence += WEXITSTATUS(childStatus);
	}
	if (childpid == 0) {
		if (isTextFile(direntName))
			totalOccurence += searchInFile(direntName, string, logFile);
		else if (isDirectory(direntName))
			totalOccurence += searchInDirectory(direntName, string, logFile);
		dirp = NULL;
		direntp = NULL;
		/* toplam bulunma sayısının parenta gönderilmesi */
		exit(totalOccurence); 
	}
	closedir(dirp);
	dirp = NULL;
	direntp = NULL;
	return totalOccurence;
}

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
int searchInFile(const char *fileName, const char *string, FILE *logFile) {

	FILE *textFile;
	int i = 0;
	int rowNumber = 1;
	int columnNumber = 1;
	int totalOccurence = 0;
	int counter = 0;
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
			fprintf(logFile, "%s: [%d,%d] %s first character is found.\n", fileName, rowNumber, columnNumber, string);
			++totalOccurence;
		}
		fseek(textFile, counter, SEEK_SET);
		if (ch != '\n')
			++columnNumber;
	}
	fclose(textFile);
	return totalOccurence;
}

/*
 * Bu fonksiyon kitaptan alınmıştır.
 * Parametre olarak verilen path isminin directory olup
 * olmadığını kontrol eder.
 * Parametreleri
 *		pathName : kontrol edilcek pathin adı
 * Return değeri : Eğer path directory ise 0 dan farklı bir değer,
 *					değilse 0 return eder.
 */	
int isDirectory(char *pathName) {

	struct stat statbuf;

	if (stat(pathName, &statbuf) == -1)
		return 0;
	else
		return S_ISDIR(statbuf.st_mode);
}

/*
 * Bu fonksiyon kendisine parametre olarak gelen fileName
 * isimli dosyanın txt dosyası olup olmadığını kontrol
 * eder.
 * Paremetreleri
 *		fileName: dosyanın adı
 * Return değeri: Eğer dosya txt dosyası ise 1,
 *                 değilse 0 return eder.
 */
int isTextFile(const char *fileName) {

	int returnVal = 0;
	int i = 0;
	int j = 0;
	char extension[] = ".txt";
	j = strlen(extension) - 1;
	for (i = strlen(fileName) - 1;
		 i >= strlen(fileName) - strlen(extension); --i) {

		if (fileName[i] == extension[j--])
			returnVal = 1;
		else {
			returnVal = 0;
			break;
		}
	}
	return returnVal;
}



