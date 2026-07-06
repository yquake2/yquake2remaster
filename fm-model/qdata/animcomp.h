#if !defined(ANIMCOMP_INC)
#define ANIMCOMP_INC

#ifdef  __cplusplus
extern "C"
{
#endif
void AnimCompressInit(int nFrames,int nVerts,int CompressedFrameSize);
void AnimSetFrame(int frame,int index,float x,float y,float z);
void AnimCompressDoit();
void AnimCompressToBytes(float *trans,float *scale,char *mat,char *ccomp,unsigned char *cbase,float *cscale,float *coffset,float *bmin,float *bmax);
void AnimCompressGetMatrix(float *mat);
void AnimCompressGetFrames(float *mat);
void AnimCompressGetBase(int i,float *x,float *y,float *z);
void AnimCompressEnd();
void DOsvdPlane(float *pnts,int npnts,float *n,float *base);
#ifdef  __cplusplus
}
#endif

#endif

