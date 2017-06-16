/*------------MATRIX OPERATION LIBRARY---------------------------------------*/
/*                                                                           */
/*	Author:Efkan DURAKLI                                                     */
/*	Date:16/04/2017                                                          */
/*                                                                           */
/* References:                                                               */
/*  1- http://www.songho.ca/dsp/convolution/convolution.html                 */
/*  2- https://www.cs.rochester.edu/~brown/Crypto/assts/projects/adj.html    */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MATRIX_SIZE 20

/* size x size boyutunda random matrsi üreten fonksiyon*/
void createRandomMatrix(double arr[][20], int size) ; 

/* size x size boyutunda matrisin determinantını hesaplayan fonksiyon*/
double determinantOfMatrix(double arr[][20], int size); 

/* size x size boyutunda matrisin transpozunu alan fonksiyon */
void transpose(double arr[][20], int size); 

/* size x size boyutunda matrisin kofaktör matrisini bulan fonksiyon */
void cofactor(double arr[][20], double b[][20], int size);

/* size x size boyutunda matrisin tersini alan fonksiyon */
void inverseOfMatrix(double arr[][20], double b[][20], int size); 

/* size x size boyutunda matrisin shifted inverse matrisini bulan fonksiyon */
void shiftedInverseMatrix(double arr[][20], double shifted[][20], int size); 

/* size x size boyutunda matrisin concolutionunu bulan fonksiyon */
void convolution2d(double arr[][20], double conv[][20], int size);

/* size x size boyutunda matrisin convolution matrisini bulan fonksiyon */
void convolutionOfMatrix(double arr[][20], double conv[][20], int size);