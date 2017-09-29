/*------------NUMERICAL METHODS LIBRARY----------------------------------------*/
/*                                                                             */
/*	Author:Efkan DURAKLI                                                   */
/*	Date:31/05/2017                                                        */
/*                                                                             */
/* Bu kütüphanedeki fonksiyonlardan bazıları Numericial Recipies in C          */
/* kitabından ufak değişiklikler yapılarak alınmıştır.                         */
/* Kaynaklar aşağıdadır.                                        	       */
/*                                                                             */
/* References:                                                                 */
/*  1- Numerical Recipes in C (The Art of Scientific Computing Second Edition) */
/*-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#define MAXSIZE 100
#define NR_END 1
#define FREE_ARG char*
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
static double dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
		(dmaxarg1) : (dmaxarg2))
static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
		(iminarg1) : (iminarg2))
static double maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
        (maxarg1) : (maxarg2))
static double sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)
#define TINY 1.0e-20


typedef struct {
	double A[MAXSIZE][MAXSIZE];
	double b[MAXSIZE];
	double x[4][MAXSIZE];
	double error[4];
}SendClient_t;

double **dmatrix(int nrl, int nrh, int ncl, int nch);

double *dvector(int nl, int nh);

void free_dvector(double *v, int nl, int nh);

void SingularValueDecomposition(double a[][MAXSIZE], int m, int n, double w[], double v[][MAXSIZE]);

void solveWithSvd(double u[][MAXSIZE], double w[], double v[][MAXSIZE], int m, int n, double b[], double x[]);

double calculateErrorNorm(double a[][MAXSIZE], double x[], double b[], int m, int n); 

double averageConnectionTime(int a[], int n); 

double calculateStandardDeviation(int a[], double average, int n);

void createRandomMatrices(double a[][MAXSIZE], double b[], int m, int n); 

void rsolv(double a[][MAXSIZE], int n, double d[], double b[]);

void solveWithQR(double a[][MAXSIZE], int n, double c[], double d[], double b[]);

void qrdcmp(double a[][MAXSIZE], int n, double *c, double *d, int *sing);

void lubksb(double a[][MAXSIZE], int n, int *indx, double b[]);

void inverseOfMatrix(double a[][MAXSIZE], double y[][MAXSIZE], int n);

void luDecomposition(double a[][MAXSIZE], int n, int *indx, double *d);

void nrerror(char error_text[]);

void transpose(double a[][MAXSIZE], double b[][MAXSIZE], int m, int n);

void multiplyMatrix(double a[][MAXSIZE], double b[][MAXSIZE], double result[][MAXSIZE], int m, int n, int p);

void solveWithInverseMethod(double A[][MAXSIZE], double b[], double x[], int m, int n);
