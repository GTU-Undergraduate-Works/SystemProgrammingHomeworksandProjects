#include "methods.h"

/* 
	Bu fonksiyon A matrisini b vektörünü parametre olarak alır
	ve hatanın normunu return eder
*/
double calculateErrorNorm(double a[][MAXSIZE], double x[], double b[], int m, int n) {


	int i,j;
	double multiply[MAXSIZE];
	double error[MAXSIZE];
	double sum = 0.0;
	double y[MAXSIZE][MAXSIZE];

	for (i = 1; i <= m; i++) {

		for (j = 1; j <= n; j++) 
			sum = sum + a[i][j] * x[j];
		multiply[i] = sum;
		sum = 0.0;
	}

	for (i = 1; i <= m; i++) 
		error[i] = multiply[i] - b[i];
	
	for (i = 1; i <= m; i++) {

		for (j = 1; j <= m; j++) 
			y[i][j] = error[i]*error[j];
	}
	sum = 0.0;
	for (i = 1; i <= m; i++) {

		for (j = 1; j <= m; j++) 
			sum = sum + pow(y[i][j], 2);
	}

	return sqrt(sum);
}

/* 
	Bu fonksiyon parametre oalrak aldığı arrayin elemanlarının 
	ortalamasını hesaplar.
*/
double averageConnectionTime(int a[], int n) {

	int sum = 0;
	int i = 0;

	for (i = 1; i <= n; i++)
		sum = sum + a[i];

	return (double)sum/n;
}

/* 
	Bu fonksiyon parametre olarak aldığı arrayin elemanlarının
	standard sapmasını hesaplar
*/
double calculateStandardDeviation(int a[], double average, int n) {

	double sum = 0.0;
	int i = 0;

	for (i = 1; i <= n; i++) 
		sum = sum + (pow(a[i], 2));

	return sqrt((sum - n*pow(average, 2))/n-1);
}
/* 
	Bu fonksiyon verilen boyutlarda random matris ve vektör oluşturur
*/
void createRandomMatrices(double a[][MAXSIZE], double b[], int m, int n) {

	int i = 0; 
	int j = 0;
	double temp;
	unsigned int seed = pthread_self() + getpid();
	for (i = 1; i <= m; i++) {
		b[i] = (rand_r(&seed) + 0.5)/(RAND_MAX + 1.0);
		for (j = 1; j <= n; j++)  
			a[i][j] = (rand_r(&seed) + 0.5)/(RAND_MAX + 1.0);
	}
}

/* aşağıdaki fonksiyonlar dışardan alınmıştır.*/

void solveWithSvd(double u[][MAXSIZE], double w[], double v[][MAXSIZE], int m, int n, double b[], double x[]) {

	int jj,j,i;
	double s,*tmp;
	tmp=dvector(1,n);
	for (j=1;j<=n;j++) {
		s=0.0;
		if (w[j]) {		/* Nonzero result only if w j is nonzero. */
			for (i=1;i<=m;i++) s += u[i][j]*b[i];
			s /= w[j];		/* This is the divide by w j . */
		}
		tmp[j]=s;
	}
	for (j=1;j<=n;j++) {	/* Matrix multiply by V to get answer. */
 		s=0.0;
		for (jj=1;jj<=n;jj++) s += v[j][jj]*tmp[jj];
		x[j]=s;
	}
	free_dvector(tmp,1,n);
}


double **dmatrix(int nrl, int nrh, int ncl, int nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	int i,nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;
	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	m += NR_END;
	m -= nrl;
	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;
	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}

double *dvector(int nl, int nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;
	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	return v-nl+NR_END;
}

void free_dvector(double *v, int nl, int nh)
/* free a double vector allocated with dvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

double pythag(double a, double b)
/* compute (a2 + b2)^1/2 without destructive underflow or overflow */
{
	double absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+(absb/absa)*(absb/absa));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+(absa/absb)*(absa/absb)));
}

/******************************************************************************/
void SingularValueDecomposition(double a[][MAXSIZE], int m, int n, double w[], double v[][MAXSIZE])
/*******************************************************************************
Given a matrix a[1..m][1..n], this routine computes its singular value
decomposition, A = U.W.VT.  The matrix U replaces a on output.  The diagonal
matrix of singular values W is output as a vector w[1..n].  The matrix V (not
the transpose VT) is output as v[1..n][1..n].
*******************************************************************************/
{
	int flag,i,its,j,jj,k,l,nm;
	double anorm,c,f,g,h,s,scale,x,y,z,*rv1;

	rv1=dvector(1,n);
	g=scale=anorm=0.0; /* Householder reduction to bidiagonal form */
	for (i=1;i<=n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) {
			for (k=i;k<=m;k++) scale += fabs(a[k][i]);
			if (scale) {
				for (k=i;k<=m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) {
			for (k=l;k<=n;k++) scale += fabs(a[i][k]);
			if (scale) {
				for (k=l;k<=n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
		anorm = DMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n;i>=1;i--) { /* Accumulation of right-hand transformations. */
		if (i < n) {
			if (g) {
				for (j=l;j<=n;j++) /* Double division to avoid possible underflow. */
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n);i>=1;i--) { /* Accumulation of left-hand transformations. */
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) {
			g=1.0/g;
			for (j=l;j<=n;j++) {
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) { /* Diagonalization of the bidiagonal form. */
		for (its=1;its<=30;its++) {
			flag=1;
			for (l=k;l>=1;l--) { /* Test for splitting. */
				nm=l-1; /* Note that rv1[1] is always zero. */
				if ((double)(fabs(rv1[l])+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((double)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0; /* Cancellation of rv1[l], if l > 1. */
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((double)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) { /* Convergence. */
				if (z < 0.0) { /* Singular value is made nonnegative. */
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) printf("no convergence in 30 svdcmp iterations");
			x=w[l]; /* Shift from bottom 2-by-2 minor. */
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0; /* Next QR transformation: */
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z; /* Rotation can be arbitrary if z = 0. */
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	free_dvector(rv1,1,n);
}


void qrDecomposition(double a[][MAXSIZE], int m, int n, double c[], double d[], int *sing) 
/******************************************************************************************************************
Constructs the QR decomposition of a[1..n][1..n] . The upper triangular matrix R is re-
turned in the upper triangle of a , except for the diagonal elements of R which are returned in
d[1..n] . The orthogonal matrix Q is represented as a product of n − 1 Householder matrices
Q 1 . . . Q n−1 , where Q j = 1 − u j ⊗ u j /c j . The ith component of u j is zero for i = 1, . . . , j − 1
while the nonzero components are returned in a[i][j] for i = j, . . . , n. sing returns as
true ( 1 ) if singularity is encountered during the decomposition, but the decomposition is still
completed in this case; otherwise it returns false ( 0 ).
/*******************************************************************************************************************/
{
	int i,j,k;
	double scale,sigma,sum,tau;

	*sing=0;
	for (k=1;k<m;k++) {
		scale=0.0;
		for (i=k;i<=n;i++) scale=FMAX(scale,fabs(a[i][k]));
		if (scale == 0.0) {
			*sing=1;
			c[k]=d[k]=0.0;
		} else {
			for (i=k;i<=n;i++) a[i][k] /= scale;
			for (sum=0.0,i=k;i<=n;i++) sum += SQR(a[i][k]);
			sigma=SIGN(sqrt(sum),a[k][k]);
			a[k][k] += sigma;
			c[k]=sigma*a[k][k];
			d[k] = -scale*sigma;
			for (j=k+1;j<=n;j++) {
				for (sum=0.0,i=k;i<=n;i++) sum += a[i][k]*a[i][j];
				tau=sum/c[k];
				for (i=k;i<=n;i++) a[i][j] -= tau*a[i][k];
			}
		}
	}
	d[n]=a[n][n];
	if (d[n] == 0.0) *sing=1;
}


void luDecomposition(double a[][MAXSIZE], int n, int *indx, double *d) 
/************************************************************************************************
Given a matrix a[1..n][1..n] , this routine replaces it by the LU decomposition of a rowwise
permutation of itself. a and n are input. a is output, arranged as in equation (2.3.14) above;
indx[1..n] is an output vector that records the row permutation effected by the partial
pivoting; d is output as ±1 depending on whether the number of row interchanges was even
or odd, respectively. This routine is used in combination with lubksb to solve linear equations
or invert a matrix.
**************************************************************************************************/
{
	int i,imax,j,k;
	double big,dum,sum,temp;
	double *vv;

	vv=dvector(1,n);
	*d=1.0;
	for (i=1;i<=n;i++) {
		big=0.0;
		for (j=1;j<=n;j++)
			if ((temp=fabs(a[i][j])) > big) big=temp;
		if (big == 0.0) nrerror("Singular matrix in routine ludcmp");
		vv[i]=1.0/big;
	}
	for (j=1;j<=n;j++) {
		for (i=1;i<j;i++) {
			sum=a[i][j];
			for (k=1;k<i;k++) sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
		}
		big=0.0;
		for (i=j;i<=n;i++) {
			sum=a[i][j];
			for (k=1;k<j;k++)
				sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
			if ( (dum=vv[i]*fabs(sum)) >= big) {
				big=dum;
				imax=i;
			}
		}
		if (j != imax) {
			for (k=1;k<=n;k++) {
				dum=a[imax][k];
				a[imax][k]=a[j][k];
				a[j][k]=dum;
			}
			*d = -(*d);
			vv[imax]=vv[j];
		}
		indx[j]=imax;
		if (a[j][j] == 0.0) a[j][j]=TINY;
		if (j != n) {
			dum=1.0/(a[j][j]);
			for (i=j+1;i<=n;i++) a[i][j] *= dum;
		}
	}
	free_dvector(vv,1,n);
}

void lubksb(double a[][MAXSIZE], int n, int *indx, double b[]) {

	int i,ii=0,ip,j;
	double sum;
	for (i=1;i<=n;i++) {
		ip=indx[i];
		sum=b[ip];
		b[ip]=b[i];
		if (ii)
			for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
		else if (sum) ii=i;
			b[i]=sum;
	}
	for (i=n;i>=1;i--) {
		sum=b[i];
		for (j=i+1;j<=n;j++) sum -= a[i][j]*b[j];
		b[i]=sum/a[i][i];
	}
}


void inverseOfMatrix(double a[][MAXSIZE], double y[][MAXSIZE], int n) {

	double d, col[MAXSIZE];
	int i,j,indx[MAXSIZE];

	luDecomposition(a, n, indx, &d);
	for(j=1;j<=n;j++) {
		for(i=1;i<=n;i++) col[i]=0.0;
		col[j]=1.0;
		lubksb(a,n,indx,col);
		for(i=1;i<=n;i++) y[i][j]=col[i];
	}
}

void nrerror(char error_text[])
/* Numerical Recipes standard error handler */
{
	fprintf(stderr,"Numerical Recipes run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	exit(1);
}

void transpose(double a[][MAXSIZE], double b[][MAXSIZE], int m, int n) {
	int i, j;
	for (i = 1; i <= m;i++) {

		for (j = 1; j <= n; j++)
			b[i][j] = a[j][i];
	}
}

void multiplyMatrix(double a[][MAXSIZE], double b[][MAXSIZE], double result[][MAXSIZE], int m, int n, int p) {

    int i,j,k;

    for (i = 1; i <= m; i++) {
        for (j = 1; j <= p; j++) 
            result[i][j] = 0.0;
    }
    for (i = 1; i <= m; i++) {
        for (j = 1; j <= p; j++) {
            for (k = 1; k <= n; k++)
                result[i][j] += a[i][k]*b[k][j];
        }
    }
}

void solveWithInverseMethod(double A[][MAXSIZE], double b[], double x[], int m, int n) {

	double tempx[100][100];
	double tempb[100][100];
	double transp[100][100];
	double inverse[100][100];
	double result1[100][100];
	double result2[100][100];
	int i,j;

	for (i = 1; i <= m; i++) 
		tempb[i][1] = b[i];
	transpose(A, transp, m, n);
	multiplyMatrix(transp, A, result1, n, m, n);
	inverseOfMatrix(result1, inverse, n);
	multiplyMatrix(transp, tempb, result2, n, m, 1);
	multiplyMatrix(inverse, result2, tempx, n, n, 1);
	for (i = 1; i <= m; i++)
		x[i] = tempx[i][1];
}

void qrdcmp(double a[][MAXSIZE], int n, double *c, double *d, int *sing)
/*************************************************************************************************************
Constructs the QR decomposition of a[1..n][1..n] . The upper triangular matrix R is re-
turned in the upper triangle of a , except for the diagonal elements of R which are returned in
d[1..n] . The orthogonal matrix Q is represented as a product of n − 1 Householder matrices
Q 1 . . . Q n−1 , where Q j = 1 − u j ⊗ u j /c j . The ith component of u j is zero for i = 1, . . . , j − 1
while the nonzero components are returned in a[i][j] for i = j, . . . , n. sing returns as
true ( 1 ) if singularity is encountered during the decomposition, but the decomposition is still
completed in this case; otherwise it returns false ( 0 ).
***************************************************************************************************************/
{
	int i,j,k;
	double scale,sigma,sum,tau;

	*sing=0;
	for (k=1;k<n;k++) {
		scale=0.0;
		for (i=k;i<=n;i++) scale=FMAX(scale,fabs(a[i][k]));
		if (scale == 0.0) {
			*sing=1;
			c[k]=d[k]=0.0;
		} else {
			for (i=k;i<=n;i++) a[i][k] /= scale;
			for (sum=0.0,i=k;i<=n;i++) sum += SQR(a[i][k]);
			sigma=SIGN(sqrt(sum),a[k][k]);
			a[k][k] += sigma;
			c[k]=sigma*a[k][k];
			d[k] = -scale*sigma;
			for (j=k+1;j<=n;j++) {
				for (sum=0.0,i=k;i<=n;i++) sum += a[i][k]*a[i][j];
				tau=sum/c[k];
				for (i=k;i<=n;i++) a[i][j] -= tau*a[i][k];
			}
		}
	}	
	d[n]=a[n][n];
	if (d[n] == 0.0) *sing=1;
}

void solveWithQR(double a[][MAXSIZE], int n, double c[], double d[], double b[])
/*******************************************************************************************
Solves the set of n linear equations A · x = b. a[1..n][1..n] , c[1..n] , and d[1..n] are
input as the output of the routine qrdcmp and are not modified. b[1..n] is input as the
right-hand side vector, and is overwritten with the solution vector on output.
*******************************************************************************************/
{

	int i,j;
	double sum,tau;
	for (j=1;j<n;j++) {
		for (sum=0.0,i=j;i<=n;i++) sum += a[i][j]*b[i];
		tau=sum/c[j];
		for (i=j;i<=n;i++) b[i] -= tau*a[i][j];
	}
	rsolv(a,n,d,b);
}

void rsolv(double a[][MAXSIZE], int n, double d[], double b[])
/************************************************************************************************
Solves the set of n linear equations R · x = b, where R is an upper triangular matrix stored in
a and d . a[1..n][1..n] and d[1..n] are input as the output of the routine qrdcmp and
are not modified. b[1..n] is input as the right-hand side vector, and is overwritten with the
solution vector on output.
*************************************************************************************************/
{
	int i,j;
	double sum;
	b[n] /= d[n];
	for (i=n-1;i>=1;i--) {
		for (sum=0.0,j=i+1;j<=n;j++) sum += a[i][j]*b[j];
		b[i]=(b[i]-sum)/d[i];
	}
}





