// 64-bit arithmetic

#pragma list(off)

int sprintl(char *dest, char *fmt, long *ll);

char* ssprintl(char *dest, char *fmt, long *ll);

long cpy64(int *dd, int *ss);

long shl64(int *vv, int *rr);

long sar64(int *vv, int *rr);

long shr64(int *vv, int *rr);

long iadd64(int *aa, int *bb, int *cc);

long isub64(int *aa, int *bb, int *cc);

long neg64(int *vv, int *rr);

long mul64(int *aa, int *bb, int *rr);

long div64(int *zhl, int *nnr, int *quo, int *rst);

#pragma list(on)
