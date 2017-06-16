#include "matrix.h"

void createRandomMatrix(double arr[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;
  double det = 0.0;


  srand((unsigned) time(NULL));
  while (det == 0) {
    for (i = 0; i < size; i++) {
      for (j = 0; j < size; j++)
        arr[i][j] = (rand() % 9) + 1;
    }
    det = determinantOfMatrix(arr, size);
  }
}

double determinantOfMatrix(double arr[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;
  int j1 = 0;
  int j2 = 0;
  int determinant = 0;
  double m[MATRIX_SIZE][MATRIX_SIZE]; /* pointer to pointer to 2d square matrix */

  if (size == 1) 
    determinant = arr[0][0];
  else if (size == 2) 
    determinant = arr[0][0] * arr[1][1] - arr[1][0] * arr[0][1];
  else {
    determinant = 0;
    for (j1 = 0; j1 < size; j1++) {
  
      for (i = 1; i < size; i++) {
        j2 = 0;
        for (j = 0; j < size; j++) {
          if (j == j1) continue;
          m[i-1][j2] = arr[i][j];
          j2++;
        }
      }
      determinant += pow(-1, 2+j1) * arr[0][j1] * determinantOfMatrix(m, size - 1);
    }
  }
  return determinant;
}


void transpose(double arr[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;
  double temp = 0.0;

  for (i = 1; i < size; i++) {
    for (j = 0; j < i; j++) {
      temp = arr[i][j];
      arr[i][j] = arr[j][i];
      arr[j][i] = temp;
    }
  }
}

void cofactor(double arr[][MATRIX_SIZE], double b[][MATRIX_SIZE], int size) {

  int i = 0, j = 0;
  int ii = 0, jj = 0;
  int i1 = 0, j1 = 0;
  double determinant = 0.0;
  double c[20][20];

  for (j = 0; j < size; j++) {
    for (i = 0; i < size; i++) {

      i1 = 0;
      for (ii = 0; ii < size; ii++) {
        if (ii == i)
          continue;
        j1 = 0;
        for (jj = 0; jj < size; jj++) {
          if (jj == j)
            continue;
          c[i1][j1] = arr[ii][jj];
          j1++;
        }
        i1++;
      }

      determinant = determinantOfMatrix(c, size-1);
      b[i][j] = pow(-1.0, i+j+2.0) * determinant;
    }
  }
}

void inverseOfMatrix(double arr[][MATRIX_SIZE], double b[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;

  cofactor(arr, b, size);
  transpose(b, size);

  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++)
      b[i][j] = (1/determinantOfMatrix(arr, size)) * b[i][j];

  }
}

void shiftedInverseMatrix(double arr[][MATRIX_SIZE], double shifted[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;
  int m = 0;
  int n = 0;
  double arr1[10][MATRIX_SIZE];
  double inverse[MATRIX_SIZE][MATRIX_SIZE];


  if (size == 2) {
    for (i = 0; i < size; i++) {
      for (j = 0; j < size; j++)
        shifted[i][j] = 1/arr[i][j];
    }
  } else {

    m = 0; 
    for (i = 0; i < size/2; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    inverseOfMatrix(arr1, inverse, size/2);
    m = 0;
    for (i = 0; i < size/2;  i++) {
      n = 0; 
      for (j = 0; j < size/2; j++) {
        shifted[i][j] = inverse[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = 0; i < size/2; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    inverseOfMatrix(arr1, inverse, size/2);
    m = 0;
    for (i = 0; i < size/2; i++) {
      n = 0; 
      for (j = size/2; j < size; j++) {
        shifted[i][j] = inverse[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      } 
      m++;
    }
    inverseOfMatrix(arr1, inverse, size/2);
    m = 0; 
    for ( i = size/2; i < size; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        shifted[i][j] = inverse[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    inverseOfMatrix(arr1, inverse, size/2);
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        shifted[i][j] = inverse[m][n];
        n++;
      }
      m++;
    }
  }
}

void convolution2d(double arr[][MATRIX_SIZE], double conv[][MATRIX_SIZE], int size) {

  int kernel[3][3] = {{1,1,-1},{-1,1,-1},{1,1,1}};
  int i,j,m,n,mm,nn,ii,jj;



  for (i = 0; i < size; ++i) {
    for (j = 0; j < size; ++j) {
      conv[i][j] = 0;
      for (m = 0; m < 3; ++m) {
        mm = 2 - m;
        for (n = 0; n < 3; ++n) {
          nn = 2 - n;
          ii = i + (m - 1);
          jj = j + (n - 1);
          if (ii >= 0 && ii < size && jj >= 0 && jj < size)
            conv[i][j] += arr[ii][jj]*kernel[mm][nn];
        }
      }
    }
  }
}

void convolutionOfMatrix(double arr[][MATRIX_SIZE], double conv[][MATRIX_SIZE], int size) {

  int i = 0;
  int j = 0;
  int m = 0;
  int n = 0;
  double arr1[10][MATRIX_SIZE];
  double convolution[MATRIX_SIZE][MATRIX_SIZE];


  if (size == 2) {
    for (i = 0; i < size; i++) {
      for (j = 0; j < size; j++)
        conv[i][j] == arr[i][j];
    }
  } else {

    m = 0; 
    for (i = 0; i < size/2; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    convolution2d(arr1, convolution, size/2);
    m = 0;
    for (i = 0; i < size/2;  i++) {
      n = 0; 
      for (j = 0; j < size/2; j++) {
        conv[i][j] = convolution[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = 0; i < size/2; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    convolution2d(arr1, convolution, size/2);
    m = 0;
    for (i = 0; i < size/2; i++) {
      n = 0; 
      for (j = size/2; j < size; j++) {
        conv[i][j] = convolution[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      } 
      m++;
    }
    convolution2d(arr1, convolution, size/2);
    m = 0; 
    for ( i = size/2; i < size; i++) {
      n = 0;
      for (j = 0; j < size/2; j++) {
        conv[i][j] = convolution[m][n];
        n++;
      }
      m++;
    }
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        arr1[m][n] = arr[i][j];
        n++;
      }
      m++;
    }
    convolution2d(arr1, convolution, size/2);
    m = 0; 
    for (i = size/2; i < size; i++) {
      n = 0;
      for (j = size/2; j < size; j++) {
        conv[i][j] = convolution[m][n];
        n++;
      }
      m++;
    }
  }
}





