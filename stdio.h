// Header for mini-c
#pragma list(off)

int fopen(char *fname, char *attrib);
int fclose(int *handle);
int fflush(int *handle);
int fseek(int *handle, int offset);
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

#pragma list(on)
// end
