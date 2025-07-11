#define pi 3.141592654
double cos(double x)
{
    int index=1;
    while(x<-pi)
    {
        index*=-1;
        x+=pi;
    }
    while(x>pi)
    {
        index*=-1;
        x-=pi;
    }
    double result = 1.0; // 第一项是1
    double term = 1.0;   // 当前项的值
    int i;
 
    for (i = 1; i <= 10; i++) {
        // 计算当前项的分母和分子
        term *= -x * x / (2 * i * (2 * i - 1)); // 计算余弦函数每一项的系数
        result += term; // 累加每一项
    }
 
    return result*index;
}

double sin(double x)
{
    return cos(pi/2-x);
}

void rfft(double *x, int n)
{
	int i, j, k, m, i1, i2, i3, i4, n1, n2, n4;
	double a, e, cc, ss, xt, t1, t2;

	for(j = 1, i = 1; i < 16; i++) {
		m = i;
		j = 2 * j;
		if(j == n) break;
	}
	n1 = n - 1;
	for(j = 0, i = 0; i < n1; i++) {
		if(i < j) {
			xt = x[j];
			x[j] = x[i];
			x[i] = xt;
		}
		k = n / 2;
		while(k < (j + 1)) {
			j = j - k;
			k = k / 2;
		}
		j = j + k;
	}
	for(i = 0; i < n; i += 2) {
		xt = x[i];
		x[i] = xt + x[i + 1];
		x[i + 1] = xt - x[i + 1];
	}
	n2 = 1;
	for(k = 2; k <= m; k++) {
		n4 = n2;
		n2 = 2 * n4;
		n1 = 2 * n2;
		e = 6.28318530718 / n1;
		for(i = 0; i < n; i += n1) {
			xt = x[i];
			x[i] = xt + x[i + n2];
			x[i + n2] = xt - x[i + n2];
			x[i + n2 + n4] = -x[i + n2 + n4];
			a = e;
			for(j = 1; j <= (n4-1); j++) {
				i1 = i + j;
				i2 = i - j + n2;
				i3 = i + j + n2;
				i4 = i - j + n1;
				cc = cos(a);
				ss = sin(a);
				a = a + e;
				t1 = cc * x[i3] + ss * x[i4];
				t2 = ss * x[i3] - cc * x[i4];
				x[i4] = x[i2] - t2;
				x[i3] = -x[i2] - t2;
				x[i2] = x[i1] - t1;
				x[i1] = x[i1] + t1;
			}
		}
	}
}

double sqrt(double x)
{
	if(x<0) x*=-1;
	double left=0, right=x>1?x:1;
	double med=(left+right)/2;
	while(right-left>1e-6)
	{
		if(med*med>x)
			right=med;
		else 
			left=med;
		med=(left+right)/2;
	}
	return (left+right)/2;
}

double sqr_pls(double x1, double x2)
{
    return sqrt(x1 * x1 + x2 * x2);
}

