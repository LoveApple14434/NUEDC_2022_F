#ifndef __RFFT_H
#define __RFFT_H
/************************************
	x		----长度为n。开始时存放要变换的实数据，最后存放变换结果的前n/2+1个值，
				其存储顺序为[Re(0),Re(1),...,Re(n/2),Im(n/2-1),...,Im(1)]。
				其中Re(0)=X(0),Re(n/2)=X(n/2)。根据X(k)的共轭对称性，很容易写
				出后半部分的值。
	n		----数据长度，必须是2的整数次幂，即n=2^m。
************************************/
void rfft(double *x, int n);
double sqr_pls(double x1, double x2);
double sqrt(double);
#endif