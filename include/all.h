// Header for mini-c
//#pragma list(off)

int fopen(/*char *fname, char *attrib*/);
int fflush(/*int *handle*/);
int fseek(/*int *handle, int offset*/);
int feof();
int printf();
int sprintf();
int fprintf();
int fgetc();
int ungetc();

int scanf();
int sscanf();
int fscanf();
int puts();
int fputs();

int _getch();

int open(char *fname, int attr);
int close(int fd);
int dup2(int fd);
int read(int fd, int cnt, int size, char *buffer);
int write(int fd, int cnt, int size, char *buffer);
int flush(int fd);

// end
// Header for mini-c

int malloc();
int calloc();
int free();

int atoi(char *s);
int itoa(int v, int b);

int exit(int ec);

// end
// Header for mini-c

int memset();
int memcpy();
int isalpha();
int isdigit();
int isalnum();
int strlen();
int strcmp();
int strchr();
int strcpy();
int strdup();

// header for mini-c

int _kbhit();

enum (false, true);

//#pragma list(on)
// end
