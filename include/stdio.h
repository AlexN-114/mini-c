// Header for mini-c
#pragma list(off)

FILE *stdin;
FILE *stdout;
FILE *stderr;

char *__DATE__;
char *__TIME__;

int fopen(char *fname, char *attrib);
int fclose(int *handle);
int fflush(int *handle);
int fseek(int *handle, int offset);
int feof(int *handle);
int printf();
int sprintf();
int fprintf();
int fgetc(int *handle);
int ungetc(int *handle);

int scanf();
int sscanf();
int fscanf();
int puts();
int fputs();

int _getch();

enum 
{
_O_RDONLY = 0x0000,_O_WRONLY = 0x0001,_O_RDWR = 0x0002,_O_APPEND = 0x0008,_O_CREAT = 0x0100,_O_TRUNC = 0x0200,_O_EXCL = 0x0400,_O_TEXT = 0x4000,_O_BINARY = 0x800,_O_WTEXT = 0x10000,_O_U16TEXT = 0x20000,_O_U8TEXT = 0x40000
};
//#define _O_ACCMODE (_O_RDONLY|_O_WRONLY|_O_RDWR)

enum 
{
SEEK_CUR = 1,SEEK_END = 2,SEEK_SET = 0
};

enum
{
STDIN_FILENO  = 0,STDOUT_FILENO = 1,STDERR_FILENO = 2
};

int open(char *fname, int attr);
int close(int fd);
int dup2(int fd);
int read(int fd, int cnt, int size, char *buffer);
int write(int fd, int cnt, int size, char *buffer);
int flush(int fd);

#pragma list(on)
// end
