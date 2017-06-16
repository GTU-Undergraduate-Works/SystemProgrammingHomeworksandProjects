/*********************************************************************************
 *       HW1_161044086_EFKAN_DURAKLI.c                                           *
 *       System Programming Homework1                                            *
 *		 Author: Efkan Duraklı                                                   *
 *       Date: 02.03.2017                                                        *
 *-------------------------------------------------------------------------------*
 * Bu program verilen dosya içerisinde verilen kelimeyi arar.Kelime aranırken    *
 * kelimeyi bölen whitespace karakterler igonere edilir.Kelimenin ilk harfinin   *
 * dosyada bulunduğu satır,sütun ve kelimenin dosyada toplam bulunma sayısı      *
 * ekrana yazdırılır.                                                            *
 ********************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* constant macro for whitespace charactesrs */
#define WHITESPACE ch == '\t' || ch == ' ' || ch == '\n'

int seacrchInFile(const char *fileName, const char *string);

int main(int argc, const char *argv[]) {

	int totalOccurence;

	if (argc != 3) {	/* check for valid number of command-line arguments */
		fprintf(stderr, "Usage: %s string filename\n", argv[0]);
		return -1;
	}
	if (-1 != (totalOccurence = seacrchInFile(argv[2], argv[1])))
		fprintf(stderr, "Total %d '%s' found\n", totalOccurence, argv[1]);
	return 0;
}

/*
 *  Bu fonksiyon verilen dosya ismindeki dosyada istenilen kelimeyi arar.
 *  Kelimeyi bölen boşluk,tab ve newline karakterleri varsa onları ignore eder.
 *  Kelimenin ilk harfinin dosyda hangi satır ve sütunda geçtiğini ekrana yazar.
 *  Paremetreleri
 *		fileName : kelimenin aranacaği dosyanın ismi.
 *		string   : aranacak kelime.
 *  Return değeri
 *      Kelimenin dosyada bulunma sayısını return eder.
 *      Hata durumunda -1 return eder.
 */
int seacrchInFile(const char *fileName, const char *string) {

	FILE *filePointer;
	int i = 0;
	int rowNumber = 1;
	int columnNUmber = 1;
	int totalOccurence = 0;
	int counter = 0;
	char ch;

	if (NULL == (filePointer = fopen(fileName, "r"))) {
		fprintf(stderr, "File <%s> not found in directory\n", fileName);
		return -1;
	
	}
	/* dosyanın içeriği karakter karakter okunur. */
	for (ch = fgetc(filePointer); ch != EOF; ch = fgetc(filePointer)) { 
		++counter;
		 /* yeni satıra geçildiğinde satır sayısı ve sütun sayısı güncellenir. */
		if (ch == '\n') {   
			++rowNumber;
			columnNUmber = 1;
		}
		/* aranacak kelimenin ilk harfiyle aynı olan bir harf bulunması 
			durumunda kelimenin diğer harfleri kontrol edlir */
		for (i = 0; i < strlen(string); ++i) {  
			if (ch == string[i]) {              
				ch = fgetc(filePointer);
				/* whitespace karakterler ignore edilir.*/
				while (WHITESPACE)                
					ch = fgetc(filePointer);
			} else break;                          
		}
		/* kontrol edilen karakterler stringin boyuna eşitse kelimenin ilk harfinin bulunduğu konum ekrana yazdırlır.*/
		if (i == strlen(string)) { 
			fprintf(stderr, "'%s' found in [%d,%d]\n", string, rowNumber, columnNUmber); 
			++totalOccurence;   
		}
		/* araramaya devam etmek için kalınan yere geri götürür.*/
		fseek(filePointer, counter, SEEK_SET);   
		if (ch != '\n')
			++columnNUmber;
	}
	fclose(filePointer);
	return totalOccurence;
}