// 64-bit arithmetic

#pragma list (off)

int sprintl(char *dest, char *fmt, long *ll)
{
    asm("
    mov edx, [ebp+16]
    push dword ptr [edx+4]
    push dword ptr [edx]
    ");
    sprintf(dest, fmt);
    asm("
    add esp,8
    ");
}

long cpy64(int *dd, int *ss)
{
    asm("
    mov edi, dword ptr [ebp+12]
    mov eax, [edi]
    mov edx, [edi+4]
    mov edi, [ebp+8]
    mov [edi], eax
    mov [edi+4], edx
    ");
    return;
}

long shl64(int *vv, int *rr)
{
    asm("
    push ebx
    push ecx
    push edi
    push esi
    mov edi, dword ptr [ebp+8]
    mov eax, dword ptr [edi]
    mov edx, dword ptr [edi+4]
    shl eax, 1
    rcl edx, 1
    mov edi, dword ptr [ebp+12]
    mov dword ptr [edi], eax
    mov dword ptr [edi+4], edx
    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    return;
}

long sar64(int *vv, int *rr)
{
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov edi, dword ptr [ebp+8]
    mov eax, [edi]
    mov edx, [edi+4]
    sar edx, 1
    rcr eax, 1
    mov edi, [ebp+12]
    mov [edi], eax
    mov [edi+4], edx
    
    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    return;
}

long shr64(int *vv, int *rr)
{
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov edi, dword ptr [ebp+8]
    mov eax, [edi]
    mov edx, [edi+4]
    shr edx, 1
    rcr eax, 1
    mov edi, [ebp+12]
    mov [edi], eax
    mov [edi+4], edx

    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    return;
}


long iadd64(int *aa, int *bb, int *cc)
{
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov edi, dword ptr [ebp+8]
    mov eax, [edi]
    mov edx, [edi+4]
    mov edi, dword ptr [ebp+12]
    mov ebx, [edi]
    mov ecx, [edi+4]
    add eax, ebx
    adc edx, ecx
    mov edi, dword ptr [ebp+16]
    mov [edi], eax
    mov [edi+4], edx

    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    return;
}

long isub64(int *aa, int *bb, int *cc)
{
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov edi, dword ptr [ebp+8]
    mov eax, [edi]
    mov edx, [edi+4]
    mov edi, dword ptr [ebp+12]
    mov ebx, [edi]
    mov ecx, [edi+4]
    sub eax, ebx
    sbb edx, ecx
    mov edi, dword ptr [ebp+16]
    mov [edi], eax
    mov [edi+4], edx

    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    return;
}

long neg64(int *vv, int *rr)
{
    int nh = 0;
    int nl = 0;
    
    isub64(&nl, vv, rr);
}

//              +08      +12      +16
long mul64(int *aa, int *bb, int *rr)
{
    int rh=0;
    int rl=0;
    int mask;
    
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov dword ptr [ebp-12], 0x80000000
    
_imul02:
    lea ebx, dword ptr [ebp-8]
    shl dword ptr [ebx], 1
    rcl dword ptr [ebx+4], 1
        
    mov esi, [ebp+12]
    mov ecx, [esi+4]
    mov esi, [ebp+8]
    mov eax, [esi]
    mov edx, [esi+4]

    mov ebx, dword ptr [ebp-12]
    test ebx, ecx
    jz _imul01
    
    add dword ptr [ebp-8], eax
    adc dword ptr [ebp-4], edx
    
_imul01:
    shr dword ptr [ebp-12], 1
    jnz _imul02

    mov dword ptr [ebp-12], 0x80000000
    
_imul04:
    lea ebx, dword ptr [ebp-8]
    shl dword ptr [ebx], 1
    rcl dword ptr [ebx+4], 1

    mov esi, [ebp+12]
    mov ecx, [esi]
    mov esi, [ebp+8]
    mov eax, [esi]
    mov edx, [esi+4]

    mov ebx, dword ptr [ebp-12]
    test ebx, ecx
    jz _imul03
    
    add dword ptr [ebp-8], eax
    adc dword ptr [ebp-4], edx
    
_imul03:
    shr dword ptr [ebp-12], 1
    jnz _imul04

    mov eax, dword ptr [ebp-8]
    mov edx, dword ptr [ebp-4]
    mov edi, [ebp+16]
    mov dword ptr [edi], eax
    mov dword ptr [edi+4], edx

    pop esi
    pop edi
    pop ecx
    pop ebx
    ");
    // printf("r   : %08x%08x\n", rh, rl);
    return;
}

//               +08       +12       +16       +20
long div64(int *zhl, int *nnr, int *quo, int *rst)
{
    int rh=0;   //-04
    int rl=0;   //-08
    int cnt=0;  //-12
    int th=0;   //-16
    int tl=0;   //-20
    
    asm("
    push ebx
    push ecx
    push edi
    push esi

    mov edi, [ebp+12]
    mov eax, [edi]                  #rst
    mov edx, [edi+4]
    mov [ebp-16], edx               # th
    mov [ebp-20], eax               # tl
    
    mov edi, dword ptr [ebp+20]     # rst
    mov esi, dword ptr [ebp+8]      # zhl
    mov ebx, dword ptr [ebp+16]     # quo
    mov dword ptr [ebx], 0
    mov dword ptr [ebx+4], 0
    mov eax, dword ptr [esi]        # zhl
    mov dword ptr [edi], eax        # rst
    mov eax, dword ptr [esi+4]      # zhl
    mov dword ptr [edi+4], eax      # rst
    
    xor ecx, ecx
idiv01:
    mov eax, [esi]                  # zhl
    mov edx, [esi+4]
    
    sub eax, [ebp-20]               # tl
    sbb edx, [ebp-16]               # th
    
    test edx, 0x80000000
    jnz idiv02
    
    shl dword ptr [ebp-20], 1       # tl
    rcl dword ptr [ebp-16], 1
    inc ecx
    ja idiv01

idiv02:    
    shr dword ptr [ebp-16], 1       # tl
    rcr dword ptr [ebp-20], 1
    
    shl dword ptr [ebx], 1          # quo
    rcl dword ptr [ebx+4], 1
   
    mov eax, [edi]                  # rst
    mov edx, [edi+4]

    sub eax, [ebp-20]               # tl
    sbb edx, [ebp-16]
    
    test edx, 0x80000000
    jnz idiv03
    
    add dword ptr [ebx], 1          # quo
    adc dword ptr [ebx+4], 0
    
    mov [edi],  eax                 # rst
    mov [edi+4], edx
    
idiv03:
    dec ecx
    jnz idiv02
    
    mov eax, dword ptr [ebx]
    mov edx, dword ptr [ebx+4]

    pop esi
    pop edi
    pop ecx
    pop ebx
    ");

    return;
}

#pragma list (on)
