#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <strstream>
#include <string>
#include <math.h>
using namespace std;

static double at,bt,ct;
#define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
		     (ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))

  static double maxarg1,maxarg2;
#define MAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
		  (maxarg1) : (maxarg2))
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

void ntrerror(char *s)
{
  printf("%s\n",s);
  DebugBreak();
  exit(1);
}

double *allocVect(int sz)
{
	double *ret=new double[sz];
	ret--;
	return ret;
}

void freeVect(double *ret)
{
	ret++;
	delete[] ret;
}

double **allocMatrix(int r,int c)
{
	double **ret=new double *[r];
	int i;
	for (i=0;i<r;i++)
		ret[i]=allocVect(c);
	ret--;
	return ret;
}

void freeMatrix(double **ret,int r)
{
	ret++;
	int i;
	for (i=0;i<r;i++)
		freeVect(ret[i]);
	delete[] ret;
}

void svdcmp(double** a, int m, int n, double* w, double** v)
{
  int flag,i,its,j,jj,k,l,nm;
  double c,f,h,s,x,y,z;
  double anorm=0.0,g=0.0,scale=0.0;
  double *rv1;
  void nrerror();

  if (m < n) ntrerror("SVDCMP: You must augment A with extra zero rows");
  rv1=allocVect(n);
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
	if (i != n) {
	  for (j=l;j<=n;j++) {
	    for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
	    f=s/h;
	    for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
	  }
	}
	for (k=i;k<=m;k++) a[k][i] *= scale;
      }
    }
    w[i]=scale*g;
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
	if (i != m) {
	  for (j=l;j<=m;j++) {
	    for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
	    for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
	  }
	}
	for (k=l;k<=n;k++) a[i][k] *= scale;
      }
    }
    anorm=MAX(anorm,(fabs(w[i])+fabs(rv1[i])));
  }
  for (i=n;i>=1;i--) {
    if (i < n) {
      if (g) {
	for (j=l;j<=n;j++)
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
  for (i=n;i>=1;i--) {
    l=i+1;
    g=w[i];
    if (i < n)
      for (j=l;j<=n;j++) a[i][j]=0.0;
    if (g) {
      g=1.0/g;
      if (i != n) {
	for (j=l;j<=n;j++) {
	  for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
	  f=(s/a[i][i])*g;
	  for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
	}
      }
      for (j=i;j<=m;j++) a[j][i] *= g;
    } else {
      for (j=i;j<=m;j++) a[j][i]=0.0;
    }
    ++a[i][i];
  }
  for (k=n;k>=1;k--) {
    for (its=1;its<=30;its++) {
      flag=1;
      for (l=k;l>=1;l--) {
	nm=l-1;
	if (fabs(rv1[l])+anorm == anorm) {
	  flag=0;
	  break;
	}
	if (fabs(w[nm])+anorm == anorm) break;
      }
      if (flag) {
	c=0.0;
	s=1.0;
	for (i=l;i<=k;i++) {
	  f=s*rv1[i];
	  if (fabs(f)+anorm != anorm) {
	    g=w[i];
	    h=PYTHAG(f,g);
	    w[i]=h;
	    h=1.0/h;
	    c=g*h;
	    s=(-f*h);
	    for (j=1;j<=m;j++) {
	      y=a[j][nm];
	      z=a[j][i];
	      a[j][nm]=y*c+z*s;
	      a[j][i]=z*c-y*s;
	    }
	  }
	}
      }
      z=w[k];
      if (l == k) {
	if (z < 0.0) {
	  w[k] = -z;
	  for (j=1;j<=n;j++) v[j][k]=(-v[j][k]);
	}
	break;
      }
      if (its == 30) ntrerror("No convergence in 30 SVDCMP iterations");
      x=w[l];
      nm=k-1;
      y=w[nm];
      g=rv1[nm];
      h=rv1[k];
      f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
      g=PYTHAG(f,1.0);
      f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
      c=s=1.0;
      for (j=l;j<=nm;j++) {
	i=j+1;
	g=rv1[i];
	y=w[i];
	h=s*g;
	g=c*g;
	z=PYTHAG(f,h);
	rv1[j]=z;
	c=f/z;
	s=h/z;
	f=x*c+g*s;
	g=g*c-x*s;
	h=y*s;
	y=y*c;
	for (jj=1;jj<=n;jj++) {
	  x=v[jj][j];
	  z=v[jj][i];
	  v[jj][j]=x*c+z*s;
	  v[jj][i]=z*c-x*s;
	}
	z=PYTHAG(f,h);
	w[j]=z;
	if (z) {
	  z=1.0/z;
	  c=f*z;
	  s=h*z;
	}
	f=(c*g)+(s*y);
	x=(c*y)-(s*g);
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
  freeVect(rv1);
}



void svbksb(double** u, double* w, double** v,int m, int n, double* b, double* x)
{
	int jj,j,i;
	double s,*tmp;
	tmp=allocVect(n);
	for (j=1;j<=n;j++)
	{
		s=0.0;
		if (w[j])
		{
			for (i=1;i<=m;i++)
				s += u[i][j]*b[i];
			s /= w[j];
		}
		tmp[j]=s;
	}
	for (j=1;j<=n;j++)
	{
		s=0.0;
		for (jj=1;jj<=n;jj++)
			s += v[j][jj]*tmp[jj];
		x[j]=s;
	}
	freeVect(tmp);
}

#undef SIGN
#undef MAX
#undef PYTHAG


#if 1
void DOsvd(float *a,float *res,float *comp,float *values,int nframes,int framesize,int compressedsize)
{
	int usedfs;
	if (nframes>framesize)
		usedfs=nframes;
	else
		usedfs=framesize;
	int *remap;
	int i,j;
	double **da=allocMatrix(usedfs,nframes);
	double **v=allocMatrix(nframes,nframes);
	double *w=allocVect(nframes);
	bool DOFerr = false;
	for (i=0;i<nframes;i++)
	{
		for (j=0;j<framesize;j++)
			da[j+1][i+1]=a[i*framesize+j];
		for (;j<usedfs;j++)
			da[j+1][i+1]=0.0;
	}

	svdcmp(da,usedfs,nframes,w,v);

	remap=new int[nframes];


	for (i=0;i<nframes;i++)
		remap[i]=-1;
	float mx;
	int bestat;
	for (j=0;j<compressedsize;j++)
	{
		mx=-1.0f;
		for (i=0;i<nframes;i++)
		{
			if (remap[i]<0&&fabs(w[i+1])>mx)
			{
				mx=fabs(w[i+1]);
				bestat=i;
			}
		}

		if(mx>0)
		{
			remap[bestat]=j;
		}
		else
		{
			DOFerr = true;
		}
	}

	if(DOFerr)
	{
		printf("Warning:  To many degrees of freedom!  File size may increase\n");

		for (i=0;i<compressedsize;i++)
		{
			values[i]=0;
			for (j=0;j<framesize;j++)
				res[i*framesize+j]=0;
		}
	}

	for (i=0;i<nframes;i++)
	{
		if (remap[i]<0)
			w[i+1]=0.0;
		else
		{
			values[remap[i]]=w[i+1];
			for (j=0;j<framesize;j++)
				res[remap[i]*framesize+j]=da[j+1][i+1];
		}
	}
	freeVect(w);
	freeMatrix(v,nframes);
	freeMatrix(da,framesize);
	delete[] remap;
}

#else

void DOsvd(float *a,float *res,float *comp,float *values,int nframes,int framesize,int compressedsize)
{
	int *remap;
	int i,j;
	int nrows;
	nrows=nframes;
	if (nrows<framesize)
		nrows=framesize;
	double **da=allocMatrix(nrows,framesize);
	double **v=allocMatrix(framesize,framesize);
	double *w=allocVect(framesize);
	for (j=0;j<framesize;j++)
	{
		for (i=0;i<nframes;i++)
			da[j+1][i+1]=a[i*framesize+j];
		for (;i<nrows;i++)
			da[j+1][i+1]=0.0;
	}

	svdcmp(da,nrows,framesize,w,v);

	remap=new int[framesize];


	for (i=0;i<framesize;i++)
		remap[i]=-1;
	float mx;
	int bestat;
	for (j=0;j<compressedsize;j++)
	{
		mx=-1.0f;
		for (i=0;i<framesize;i++)
		{
			if (remap[i]<0&&fabs(w[i+1])>mx)
			{
				mx=fabs(w[i+1]);
				bestat=i;
			}
		}
		assert(mx>-.5f);
		remap[bestat]=j;
	}
	// josh **DO NOT** put your dof>nframes mod here
	for (i=0;i<framesize;i++)
	{
		if (remap[i]<0)
			w[i+1]=0.0;
		else
		{
			values[remap[i]]=w[i+1];
			for (j=0;j<framesize;j++)
				res[remap[i]*framesize+j]=v[j+1][i+1];
		}
	}
	freeVect(w);
	freeMatrix(v,framesize);
	freeMatrix(da,nrows);
	delete[] remap;
}

#endif

extern "C"

void DOsvdPlane(float *pnts,int npnts,float *n,float *base)
{
	int i,j;
	double **da=allocMatrix(npnts,3);
	double **v=allocMatrix(3,3);
	double *w=allocVect(3);

	assert(npnts>=3);
	base[0]=pnts[0];
	base[1]=pnts[1];
	base[2]=pnts[2];
	for (i=1;i<npnts;i++)
	{
		for (j=0;j<3;j++)
			base[j]+=pnts[i*3+j];
	}
	base[0]/=float(npnts);
	base[1]/=float(npnts);
	base[2]/=float(npnts);

	for (i=0;i<3;i++)
	{
		for (j=0;j<npnts;j++)
			da[j+1][i+1]=pnts[j*3+i]-base[i];
	}

	svdcmp(da,npnts,3,w,v);
	float mn=1E30f;
	int bestat;
	for (i=0;i<3;i++)
	{
		if (fabs(w[i+1])<mn)
		{
			mn=fabs(w[i+1]);
			bestat=i;
		}
	}
	n[0]=v[1][bestat+1];
	n[1]=v[2][bestat+1];
	n[2]=v[3][bestat+1];
	freeVect(w);
	freeMatrix(v,3);
	freeMatrix(da,npnts);
}
