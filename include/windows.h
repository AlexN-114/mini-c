// Header for mini-c
#pragma list(on)

// winbase.h
int  GetCurrentDirectory(int  nBufferLength, char *lpBuffer);
bool SetCurrentDirectory(char *lpPathName);
bool SetSearchPathMode(int Flags);
int SearchPath(char *Path,char *FileName,char *Extension,int BufferLength,char *Buffer,char **FilePart);

int Sleep@4(int t);
int Sleep(int t) {Sleep@4(t);}

#pragma list(on)


