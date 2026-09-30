//-------------------------------//
// mini-c, by Sam Nipps (c) 2015 //
// MIT license                   //
// AlexN-114             2025/26 //
//-------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>

//No enums :(
int ptr_size = 4;
int word_size = 4;

char * outputname;
FILE * output;

char * inputname;
FILE * input;

char * old_iname;
int old_line;
FILE * old_input = 0;

//==== Lexer ====
int curln;
char curch;
char nxtch;
char *line_cache;
char *line_pointer;
char tc = ':';
bool list = true;
bool source = true;

char * home;
char * buffer;
int buflength;
int token;
int flag;
enum {getadr = 1, getvalue = 2, array = 4};
enum {_char = 0x0001, _short = 0x0002, _int = 0x0004, _long = 0x0010, _bool = 0x0020, _struct = 0x0100, _type = 0x0200, _unsigned = 0x00800, _ptr = 0x01000, _array = 0x010000};

enum {token_other = 0, token_ident, token_char, token_short, token_int, token_long, token_str, token_ptr};
// int token_other = 0;
// int token_ident = 1;
// int token_char = 2;
// int token_short = 3;
// int token_int = 4;
// int token_long = 5;
// int token_str = 6;
// int token_ptr = 7;


// Prototypes
void error(char * format);
char next_char();
void next();
bool see(char *look);
void line();
void decl(int kind);
void do_preprocess();
int _sizeof();
int sym_lookup(char ** table, int table_size, char * look);
void assembler();

void println()
{
    if (list)
        printf("%5d%c ", curln, tc);
}

void read_line()
{
    line_pointer = line_cache;

    do
    {
        if (feof(input))
        {
            if (old_input != 0)
            {
                fclose(input);
                input = old_input;
                inputname = old_iname;
                free(old_iname);
                curln = old_line;
                old_input = 0;
                line_pointer = line_cache;
                (line_pointer + 0)[0] = '\n';
                (line_pointer + 1)[0] = 0;
                printf("\n");
                tc = ':';
                return;
            }
            break;
        }
        else
        {
            line_pointer[0] = fgetc(input);
        }
    }
    while (((line_pointer++)[0] != '\n') && !(feof(input)));

    if (feof(input))
        (line_pointer - 1)[0] = '\n';
    line_pointer[0] = 0;
    if (source)
        fprintf(output, "# %s\n", line_cache);
    line_pointer = line_cache;
}

char next_char()
{
    if (curch == '\n')
    {
        curln++;
        println();
        if (nxtch == 0)
            read_line();
    }

    curch = line_pointer[0];
    nxtch = line_pointer[1];
    line_pointer++;
    if (list)
        printf("%c", curch);

    return curch;
}

bool prev_char(char before)
{
    //ungetc(curch, input);
    line_pointer--;
    curch = before;
    printf("\b \b");

    return false;
}

void eat_char()
{
    //The compiler is typeless, so as a compromise indexing is done
    //in word size jumps, and pointer arithmetic in byte jumps.
    (buffer + buflength++)[0] = curch;
    next_char();
}

void next()
{
    char oldch;
    //Skip whitespace
    while (curch == ' ' || curch == '\r' || curch == '\n' || curch == '\t')
        next_char();

    //Treat preprocessor lines as line comments
    if (curch == '#')
    {
        do_preprocess();
        next_char();
        next();
        return;
    }
    else if ((curch == '/' && (next_char() == '/' || prev_char('/'))))
    {
        while (curch != '\n' && !feof(input))
            next_char();

        //Restart the function (to skip subsequent whitespace, comments and pp)
        next();
        return;
    }
    else if (curch == '/' && (next_char() == '*' || prev_char('/')))
    {
        /* Read C comments */
        int allread = 0;
        while (!allread && !feof(input))
        {
            if ((oldch == '*') && (curch == '/'))
                allread = 1;
            else
            {
                oldch = curch;
                next_char();
            }
        }
        next_char();
        next();
        return;
    }

    buflength = 0;
    token = token_other;

    //Identifier or keyword
    if (isalpha(curch) || curch == '_')
    {
        token = token_ident;

        while ((isalnum(curch) || curch == '_' || curch == '@') && !feof(input))
            eat_char();

        //Integer literal
    }
    else if (curch == '0')
    {
        token = token_int;
        eat_char();
        if (curch == 'x' || curch == 'X')
        {
            eat_char();
            while ((((curch >= '0') && (curch <= '9')) ||
                    ((curch >= 'A') && (curch <= 'F')) ||
                    ((curch >= 'a') && (curch <= 'f'))) && !feof(input))
                eat_char();
        }
        else
        {
            while (isdigit(curch) && !feof(input))
                eat_char();
        }
    }
    else if (isdigit(curch))
    {
        token = token_int;

        while (isdigit(curch) && !feof(input))
            eat_char();

        //String or character literal
    }
    else if (curch == '\'' || curch == '"')
    {
        token = curch == '"' ? token_str : token_char;
        eat_char();

        while (curch != buffer[0] && !feof(input))
        {
            if (curch == '\\')
                eat_char();

            eat_char();
        }
        eat_char();

        //Operators which form a new operator when duplicated e.g. '++'
    }
    else if (curch == '+' /*|| curch == '-'*/ || curch == '=' || curch == '|' || curch == '&')
    {
        eat_char();

        if (curch == buffer[0])
            eat_char();

        //Operators which may be followed by a '='
    }
    else if (curch == '!')
    {
        eat_char();

        if (curch == '=')
            eat_char();

    }
    else if (curch == '>')
    {
        eat_char();

        if ((curch == '=') || (curch == '>'))
            eat_char();

    }
    else if (curch == '<')
    {
        eat_char();

        if ((curch == '=') || (curch == '<'))
            eat_char();
    }
    else if (curch == '-')
    {
        eat_char();

        if ((curch == '-') || (curch == '>'))
            eat_char();
    }
    else
        eat_char();

    (buffer + buflength++)[0] = 0;
}

void lex_init(char * filename, int maxlen)
{
    inputname = strdup(filename);
    input = fopen(filename, "r");

    if (input == 0)
    {
        printf("File '%s' not found!\n", filename);
        exit(2);
    }

    //Get the lexer into a usable state for the parser
    curln = 1;
    println();

    buffer = malloc(maxlen);
    line_cache = malloc(maxlen);
}

void lex_end()
{
    free(buffer);
    fclose(input);
}

//==== Parser helper functions ====

int errors;
int loop_to_inner = 0;
int break_to_inner = 0;
int next_case_inner = 0;

int curtype;
int cursize;
int curstruct;
char * bjct;
char * rgst;

int *used_type;
int *used_dref;
int used_idx = 0;

void error(char * format)
{
    fprintf(stderr, "\n%s:%d: error: ", inputname, curln);
    //Accepting an untrusted format string? Naughty!
    fprintf(stderr, format, buffer);
    errors++;
}

void require(bool condition, char * format)
{
    if (!condition)
        error(format);
}

bool see(char * look)
{
    return !strcmp(buffer, look);
}

bool waiting_for(char * look)
{
    return !see(look) && !feof(input);
}

void match(char * look)
{
    if (!see(look))
    {
        fprintf(stderr, "%s:%d: error: ", inputname, curln);
        fprintf(stderr, "expected '%s', found '%s'\n", look, buffer);
        errors++;
    }

    next();
}

bool try_match(char * look)
{
    if (see(look))
    {
        next();
        return true;

    }
    else
        return false;
}

//==== Symbol table ====
//BookMark [{Symbol table}]

char ** globals;
int * globals_type;
int * globals_size;
int * globals_inst;
int * globals_struct;
int global_no;
bool * is_fn;
int *used_fn;
int use_fn;

char ** locals;
int local_no;
int param_no;
int local_offset;
int * offsets;
int * locals_type;
int * locals_size;
int * locals_inst;
int * locals_struct;
int param_flag = 0;

char ** enum_name;
int * enum_values;
int enum_no;
int enum_count;

char **structs;
int *struct_size;
int *struct_start;
int *struct_ende;
int *struct_el_type;
int *struct_el_size;
int *struct_el_offset;
char **struct_el_name;
char *struct_nm;
int struct_no;
int struct_el_no;

void sym_init(int max)
{
    globals = malloc(ptr_size * max);
    globals_type = malloc(ptr_size * max);
    globals_size = malloc(ptr_size * max);
    globals_struct = malloc(ptr_size * max);
    globals_inst = malloc(ptr_size * max);
    global_no = 0;
    is_fn = calloc(max, ptr_size);
    used_fn = malloc(word_size * max);
    use_fn = 0;

    locals        = malloc(ptr_size * max);
    locals_type   = malloc(ptr_size * max);
    locals_size   = malloc(ptr_size * max);
    locals_struct = malloc(ptr_size * max);
    locals_inst   = malloc(ptr_size * max);
    local_no = 0;
    param_no = 0;
    local_offset = 0;
    offsets = calloc(max, word_size);

    used_type = malloc(ptr_size * max);
    used_dref = malloc(ptr_size * max);

    enum_name = malloc(ptr_size * max);
    enum_values = calloc(max, ptr_size);
    enum_no = 0;

    structs          = calloc(max, ptr_size);
    struct_size      = calloc(max, ptr_size);
    struct_start     = calloc(max, ptr_size);
    struct_ende      = calloc(max, ptr_size);
    struct_el_name   = calloc(max, ptr_size);
    struct_el_type   = calloc(max, ptr_size);
    struct_el_size   = calloc(max, ptr_size);
    struct_el_offset = calloc(max, ptr_size);
    struct_no = 0;
    struct_el_no = 0;
}

void table_end(char ** table, int table_size)
{
    int i = 0;
    int * types;

    if (table == globals)
        types = globals_type;
    else if (table == locals)
        types = locals_type;

    while (i < table_size)
    {
        // printf("%4x\t%s\n",types[i], table[i]);
        free(table[i++]);
    }
}

void sym_end()
{
    table_end(globals, global_no);
    free(globals);
    free(globals_type);
    free(globals_size);
    free(globals_inst);
    free(is_fn);
    free(used_fn);

    table_end(locals, local_no);
    free(locals);
    free(locals_type);
    free(locals_size);
    free(locals_inst);
    free(offsets);

    free(used_type);
    free(used_dref);

    free(enum_name);
    free(enum_values);

    free(structs);
    free(struct_size);
    free(struct_start);
    free(struct_ende);
    free(struct_el_type);
    free(struct_el_size);
    free(struct_el_offset);
    free(struct_el_name);
}

void new_global(char * ident)
{
    char * locBuf = malloc(100);
    int local = sym_lookup(globals, global_no, ident);
    sprintf(locBuf, "global symbol '%s' already declared\n", ident);
    if (local >= 0)
        require(is_fn[local], locBuf);
    free(locBuf);

    globals_type[global_no] = curtype;
    globals_size[global_no] = cursize;
    globals_struct[global_no] = sym_lookup(structs, struct_no, struct_nm);
    globals[global_no] = ident;
    global_no++;
}

void new_fn(char * ident)
{
    is_fn[global_no] = true;
    new_global(ident);
}

int new_local(char * ident)
{
    char * locBuf = malloc(100);
    int local = sym_lookup(locals, local_no, ident);
    sprintf(locBuf, "local symbol '%s' already declared\n", ident);
    require(local < 0, locBuf);
    free(locBuf);

    int var_index = local_no - param_no;

    locals_type[local_no] = curtype;
    locals_size[local_no] = cursize;
    locals_struct[local_no] = sym_lookup(structs, struct_no, struct_nm);
    locals[local_no] = ident;
    //The first local variable is directly below the base pointer
    offsets[local_no] = -word_size * (var_index + 1);
    // offsets[local_no] = -word_size * (var_index + 1);
    return local_no++;
}

void new_param(char * ident)
{
    param_flag = 1;
    int local = new_local(ident);

    //At and above the base pointer, in order, are:
    // 1. the old base pointer, [ebp]
    // 2. the return address, [ebp+W]
    // 3. the first parameter, [ebp+2W]
    //   and so on
    offsets[local] = word_size * (2 + param_no++);
    param_flag = 0;
}

//Enter the scope of a new function
void new_scope()
{
    table_end(locals, local_no);
    local_no = 0;
    param_no = 0;
    local_offset = 0;
}

int sym_lookup(char ** table, int table_size, char * look)
{
    int i = 0;

    while (i < table_size)
        if (!strcmp(table[i++], look))
            return i - 1;

    return -1;
}

//==== Codegen labels ====

int label_no = 0;

//The label to jump to on `return`
int return_to;

int new_label()
{
    return label_no++;
}

//==== One-pass parser and code generator ====

bool lvalue;

void needs_lvalue(char * msg)
{
    if (!lvalue)
        error(msg);

    lvalue = false;
}

void expr(int level);

//The code generator for expressions works by placing the results
//in eax and backing them up to the stack.

//Regarding lvalues and assignment:

//An expression which can return an lvalue looks head for an
//assignment operator. If it finds one, then it pushes the
//address of its result. Otherwise, it dereferences it.

//The global lvalue flag tracks whether the last operand was an
//lvalue; assignment operators check and reset it.

void factor()
{
    lvalue = false;

    if (see("true") || see("false"))
    {
        fprintf(output, "\tmov eax, %d\n", see("true") ? 1 : 0);
        next();
    }
    else if (see("sizeof"))
    {
        fprintf(output, "\tmov eax, %d\n", _sizeof());
        next();
    }
    else if (token == token_ident)
    {
        int global = sym_lookup(globals, global_no, buffer);
        int local = sym_lookup(locals, local_no, buffer);
        int enumidx = sym_lookup(enum_name, enum_no, buffer);

        require(global >= 0 || local >= 0 || enumidx >= 0, "no symbol '%s' declared\n");

        if (local >= 0)
        {
            cursize = locals_size[local];
            curtype = locals_type[local];
            curstruct = (curtype & _struct) ? locals_struct[local] : -1;
            lvalue = (curtype == _struct) ? true : false;
        }
        else if (global >= 0)
        {
            cursize = globals_size[global];
            curtype = globals_type[global];
            curstruct = (curtype & _struct) ? globals_struct[global] : -1;
            lvalue = (curtype == _struct) ? true : false;
        }
        else
        {
            cursize = word_size;
            curtype = 0;
            curstruct = 0;
        }

        used_idx++;
        used_type[used_idx] = curtype;
        used_dref[used_idx] = 0;

        next();

        if (see("=") || see("++") || see("--"))
            lvalue = true;

        if (enumidx >= 0)
        {
            fprintf(output, "\tmov eax, %d""\t# %s\n", enum_values[enumidx], enum_name[enumidx]);
        }
        else if (local >= 0)
        {
            if ((flag & getadr) || (locals_type[local] & _array))
            {
                fprintf(output,
                        "\tmov eax, ebp\n"
                        "\tadd eax, %+d\n", offsets[local]);
            }
            else if (flag & getvalue)
            {
                if (lvalue)
                {
                    fprintf(output,
                            "\tmov ebx, ebp\n"
                            "\tadd ebx, %+d\n"
                            "\tmov eax, [ebx]\n", offsets[local]);
                }
                else
                {
                    fprintf(output,
                            "\tmov ebx, ebp\n"
                            "\tadd ebx, %+d\n"
                            "\tmov ebx, [ebx]\n"
                            "\tmov eax, [ebx]\n", offsets[local]);
                    if ((locals_size[local] & 0xF000) == 0)
                    {
                        if (locals_size[local] <= 1)
                            fprintf(output, "\tcbw\n");
                        if (locals_size[local] <= 2)
                            fprintf(output, "\tcwde\n");
                    }
                }
                used_dref[used_idx] = (used_dref[used_idx] << 1) | _ptr;
            }
            else
            {
                fprintf(output, "\t%s eax, [ebp%+d]\n", lvalue ? "lea" : "mov", offsets[local]);
            }
        }
        else if (global >= 0)
        {
            if (!is_fn[global])
            {
                // fprintf(output, "\t%s eax, [_%s]\n", is_fn[global] || lvalue ? "lea" : "mov", globals[global]);
                if ((flag & getadr) || (globals_type[global] & _array))
                {
                    fprintf(output, "\tlea eax, _%s\n", globals[global]);
                }
                else if (flag & getvalue)
                {
                    if (lvalue)
                    {
                        fprintf(output, "\tlea ebx, [_%s]\n"
                                        "\tmov eax, [ebx]\n", globals[global]);
                    }
                    else
                    {
                        fprintf(output, "\tlea ebx, [_%s]\n"
                                        "\tmov ebx, [ebx]\n"
                                        "\tmov eax, [ebx]\n", globals[global]);
                        if ((globals_type[global] & 0xF000) == 0)
                        {
                            if (globals_size[global] <= 1)
                                fprintf(output, "\tcbw\n");
                            if (globals_size[global] <= 2)
                                fprintf(output, "\tcwde\n");
                        }
                    }
                    used_dref[used_idx] = (used_dref[used_idx] << 1) | _ptr;
                }
                else
                {
                    fprintf(output, "\t%s eax, [_%s]\n", lvalue ? "lea" : "mov", globals[global]);
                }
            }
            else
            {
                used_fn[use_fn++] = global;
            }
        }
    }
    else if (token == token_int || token == token_char)
    {
        fprintf(output, "\tmov eax, %s\n", buffer);
        next();
    }
    else if (token == token_str)
    {
        int str = new_label();

        fprintf(output, ".section .rodata\n"
                        "_%08d:\n", str);

        //Consecutive string literals are concatenated
        while (token == token_str)
        {
            fprintf(output, ".ascii %s\n", buffer);
            next();
        }

        fputs(".byte 0\n"
              ".section .text\n", output);

        fprintf(output, "\tmov eax, offset _%08d\n", str);

    }
    else if (try_match("("))
    {
        expr(0);
        match(")");

    }
    else
        error("expected an expression, found '%s'\n");

}

void object()
{
    int i;
    int offset;

    factor();

    while (true)
    {
        if (try_match("("))
        {
            // fputs("\tpush eax\n", output);

            int arg_no = 0;

            if (waiting_for(")"))
            {
                int start_label = new_label();
                int end_label = new_label();
                int prev_label = end_label;

                fprintf(output, "\tjmp _%08d\n", start_label);

                do
                {
                    int next_label = new_label();

                    fprintf(output, "_%08d:\n", next_label);
                    expr(0);
                    fprintf(output, "\tpush eax\n"
                                    "\tjmp _%08d\n", prev_label);
                    arg_no++;

                    prev_label = next_label;
                }
                while (try_match(","));

                fprintf(output, "_%08d:\n", start_label);
                fprintf(output, "\tjmp _%08d\n", prev_label);
                fprintf(output, "_%08d:\n", end_label);
            }

            match(")");

            use_fn--;
            // fprintf(output, "\tcall dword ptr [esp+%d]; _%s\n", arg_no * word_size,globals[used_fn[use_fn]]);
            // fprintf(output, "\tadd esp, %d\n", (arg_no + 1) * word_size);
            fprintf(output, "\tcall _%s\n", globals[used_fn[use_fn]]);
            fprintf(output, "\tadd esp, %d\n", (arg_no) * word_size);

        }
        else if (try_match("["))
        {
            int _cursize = cursize;
            int _curtype = curtype;

            fputs("\tpush eax\n", output);

            used_dref[used_idx] = (used_dref[used_idx] << 1) | _ptr;
            // fprintf(output,"# Pointer: %04x %04x %d\n", _curtype, ~used_dref[used_idx], used_idx);

            expr(0);
            match("]");

            if (see("=") || see("++") || see("--"))
                lvalue = true;

            // fprintf(output,"# Pointer: %04x %04x %d\n", _curtype, ~used_dref[used_idx-1], used_idx);

            int x = _curtype & ~used_dref[used_idx - 1] & 0xF000;

            _cursize = (x != 0) ? ptr_size : _cursize;

            if ((_cursize==1) || (_cursize==2) || (_cursize==4) || (_cursize==8))
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\t%s eax, [eax*%d+ebx] # %s\n", lvalue ? "lea" : "mov", _cursize, bjct, rgst);
            }
            else
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\timul eax, eax, %d\n"
                        "\t%s eax, [eax+ebx] # %s\n", _cursize, lvalue ? "lea" : "mov", bjct, rgst);
            }

            if (!lvalue)
            {
                if (_cursize <= 1)
                    fprintf(output, "\tcbw\n");
                if (_cursize <= 2)
                    fprintf(output, "\tcwde\n");
            }
        }
        else if (try_match("->"))
        {
            fputs("\tpush eax\n", output);

            offset = 0;

//1            fprintf(stderr,"<dbg %d> struct_start/-ende %d %d\n", __LINE__, struct_start[curstruct], struct_ende[curstruct]);
            for (i = struct_start[curstruct]; i < struct_ende[curstruct]; i++)
            {
//1                fprintf(stderr,"<dbg %d> struct_start/-ende\t%s %s\n", __LINE__, buffer, struct_el_name[i]);
                used_dref[used_idx] = (used_dref[used_idx] << 1) | _ptr;
                if (strcmp(struct_el_name[i], buffer) == 0)
                {
                    offset = struct_el_offset[i];
                    curtype = struct_el_type[i];
                    cursize = struct_el_size[i];
                    break;
                }
                require(i<struct_ende[curstruct],"'%s' is no member of the struct");

            }

            bjct = (curtype&0xF000)?"dword":(curtype & _short)?"word":(curtype & _char)?"byte":(curtype & _long)?"qword":"dword";
            rgst = (curtype&0xF000)?"eax":(curtype & _short)?"ax":(curtype &_char)?"al":(curtype & _long)?"rax":"eax";

            next();

            if (see("=") || see("++") || see("--"))
                lvalue = true;
            else
                lvalue = false;

            if (lvalue)
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\tadd ebx, %d\n"
                        "\tlea eax, %s ptr [ebx] # %04x %d\n", offset, bjct, curtype, cursize);
            }
            else
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\tadd ebx, %d\n"
                        "\tmov %s, %s ptr [ebx] # %04x %d\n", offset, rgst, bjct, curtype, cursize);

                if ((curtype & 0xF000) == 0)
                {
                    if (cursize <= 1)
                        fprintf(output, "\tcbw\n");
                    if (cursize <= 2)
                        fprintf(output, "\tcwde\n");
                }
            }

        }
        else if (try_match("."))
        {
            fputs("\tpush eax\n", output);

            offset = 0;

//1            fprintf(stderr,"<dbg %d> struct_start/-ende %d %d\n", __LINE__,  struct_start[curstruct], struct_ende[curstruct]);
            for (i = struct_start[curstruct]; i < struct_ende[curstruct]; i++)
            {
//1                fprintf(stderr,"<dbg %d> buffer struct_el_name\t%s %s\n", __LINE__, buffer, struct_el_name[i]);
                if (strcmp(struct_el_name[i], buffer) == 0)
                {
                    offset = struct_el_offset[i];
                    curtype = struct_el_type[i];
                    cursize = struct_el_size[i];
                    break;
                }
                require(i<struct_ende[curstruct],"'%s' is no member of the struct");
            }

            bjct = (curtype&0xF000)?"dword":(curtype & _short)?"word":(curtype & _char)?"byte":(curtype & _long)?"qword":"dword";
            rgst = (curtype&0xF000)?"eax":(curtype & _short)?"ax":(curtype & _char)?"al":(curtype & _long)?"rax":"eax";

            next();

            if (see("=") || see("++") || see("--"))
                lvalue = true;
            else
                lvalue = false;

            if (lvalue)
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\tadd ebx, %d\n"
                        "\tlea eax, %s ptr [ebx] # %04x %d\n", offset, bjct, curtype, cursize);
            }
            else
            {
                fprintf(output,
                        "\tpop ebx\n"
                        "\tadd ebx, %d\n"
                        "\tmov %s, %s ptr [ebx] # %04x %d\n", offset, rgst, bjct, curtype, cursize);

                if ((curtype & 0xF000) == 0)
                {
                    if (cursize <= 1)
                        fprintf(output, "\tcbw\n");
                    if (cursize <= 2)
                        fprintf(output, "\tcwde\n");
                }
            }
        }
        else
        {
            return;
        }
    }
}

void unary()
{
    if (try_match("!"))
    {
        //Recurse to allow chains of unary operations, LIFO order
        unary();

        fputs("\tcmp eax, 0\n"
              "\tmov eax, 0\n"
              "\tsete al\n", output);

    }
    else if (try_match("-"))
    {
        unary();
        fputs("\tneg eax\n", output);

    }
    else if (try_match("~"))
    {
        unary();
        fputs("\tnot eax\n", output);

    }
    else if (try_match("&"))
    {
        flag = flag | getadr;
        unary();
        flag = flag & ~getadr;
    }
    else if (try_match("*"))
    {
        used_dref[used_idx] = (used_dref[used_idx] << 1) | _ptr;
        flag = flag | getvalue;
        unary();
        flag = flag & ~getvalue;
    }
    else
    {
        //This function call compiles itself
        object();

        if (see("++") || see("--"))
        {
            //bjct = ((curtype & 0xF002) == 0x2)? "byte" : (cursize == 2)? "word" : "dword";

            fprintf(output, "\tmov ebx, eax\n"
                            "\tmov eax, [ebx]\n"
                            "\t%s dword ptr [ebx], 1  # %s\n", see("++") ? "add" : "sub", bjct);

            needs_lvalue("assignment operator '%s' requires a modifiable object\n");
            next();
        }
    }
}

void branch(bool expr);

void expr(int level)
{
    int div = 0;

    if (level == 6)
    {
        unary();
        return;
    }

    expr(level + 1);

    while (level == 5 ? see("*") || see("/") || see("%")
           : level == 4 ? see("+") || see("-") || see("|") || see("&") || see("<<") || see(">>") || see("^")
           : level == 3 ? see("==") || see("!=") || see("<") || see(">") || see("<=") || see(">=")
           : false)
    {
        if (see("/"))
            div = 1;
        if (see("%"))
            div = 2;
        if (see("<<") || see(">>"))
            div = 3;

        fputs("\tpush eax\n", output);

        char * instr = see("+") ? "add" : see("-") ? "sub" : see("|") ? "or" : see("&") ? "and" : see("*") ? "imul" : see("^") ? "xor"
                       : see("/") ? "idiv" : see("%") ? "idiv" : see("<<") ? "sal" : see(">>") ? "sar"
                       : see("==") ? "e" : see("!=") ? "ne" : see("<") ? "l" : see(">") ? "g" : see("<=") ? "le" : "ge";

        next();
        expr(level + 1);

        if (level == 5)
        {
            if (div == 0)
            {
                fprintf(output, "\tmov ebx, eax\n"
                                "\tpop eax\n"
                                "\t%s eax, ebx\n", instr);
            }
            else if (div == 1)
            {
                fprintf(output, "\tmov ebx, eax\n"
                                "\tpop eax\n"
                                "\txor edx, edx\n"
                                "\t%s ebx\n", instr);
            }
            else
            {
                fprintf(output, "\tmov ebx, eax\n"
                                "\tpop eax\n"
                                "\txor edx, edx\n"
                                "\t%s ebx\n"
                                "\tmov eax, edx\n", instr);
            }
        }
        else if (level == 4)
            if (div == 3)
            {
                fprintf(output, "\tmov ecx, eax\n"
                                "\tpop eax\n"
                                "\t%s eax, cl\n", instr);
            }
            else
            {
                fprintf(output, "\tmov ebx, eax\n"
                                "\tpop eax\n"
                                "\t%s eax, ebx\n", instr);
            }
        else
        {
            fprintf(output, "\tpop ebx\n"
                            "\tcmp ebx, eax\n"
                            "\tmov eax, 0\n"
                            "\tset%s al\n", instr);
        }
    }

    if (level == 2)
        while (see("||") || see("&&"))
        {
            int shortcircuit = new_label();

            fprintf(output, "\tcmp eax, 0\n"
                            "\tj%s _%08d\n", see("||") ? "nz" : "z", shortcircuit);
            next();
            expr(level + 1);

            fprintf(output, "_%08d:\n", shortcircuit);
        }

    if (level == 1 && try_match("?"))
        branch(true);

    if (level == 0 && try_match("="))
    {
        fputs("\tpush eax\n", output);

        bjct = "dword";
        rgst = "eax";

        if (lvalue && !param_flag && (token != token_str) && (used_type[used_idx]))
        {
            int x1 = ~used_dref[used_idx];
            int x2 = used_type[used_idx];
            int x3 = x1 & x2;
            if ((x3 & 0xf000) == 0)
                // if ((~used_dref[used_idx] & used_type[used_idx]) & 0xf000 == 0)
            {
 //               bjct = (cursize == 1) ? "byte" : (cursize == 2) ? "word" : "dword";
 //               rgst = (cursize == 1) ? "al" : (cursize == 2) ? "ax" : "eax";
                bjct = (curtype&0xF000)?"dword":(curtype & _short)?"word":(curtype & _char)?"byte":(curtype & _long)?"qword":"dword";
                rgst = (curtype&0xF000)?"eax":(curtype & _short)?"ax":(curtype & _char)?"al":(curtype & _long)?"rax":"eax";
            }
            used_idx--;
        }

        needs_lvalue("assignment requires a modifiable object\n");
        expr(level + 1);

        fprintf(output, "\tpop ebx\n"
                //"\tmov %s ptr [ebx], %s # %d %04x\n", bjct, rgst, cursize, curtype);
                        "\tmov %s ptr [ebx], %s # %d %04x\n", bjct, rgst, cursize, curtype);
    }
}

void for_loop()
{
    // labels for break and continue
    int loop_to = new_label();
    int break_to = new_label();
    int loop_to_prev = loop_to_inner;
    int break_to_prev = break_to_inner;
    int body_to = new_label();
    int incl_to = new_label();

    loop_to_inner = incl_to;
    break_to_inner = break_to;

    // for body intro
    fprintf(output, "## for loop init\n");
    match("for");
    match("(");
    if (!see(";"))
        do
        {
            expr(0);
        }
        while (try_match(","));

    match(";");

    // for body condition
    fprintf(output, "## for loop entry\n"
            "_%08d:\n", loop_to);

    if (!see(";"))
        expr(0);
    else
        fprintf(output, "\tmov eax, 1\n");

    fprintf(output, "\tcmp eax, 0\n"
                    "\tjne _%08d\n"
                    "\tjmp _%08d\n"
            //"# for loop incr loop\n"
                    "_%08d:\n", body_to, break_to, incl_to);
    match(";");

    // for body loop vars
    if (!see(")"))
        do
        {
            expr(0);
        }
        while (try_match(","));

    match(")");

    fprintf(output, //"# for loop body\n"
            "\tjmp _%08d\n"
            "_%08d:\n", loop_to, body_to);

    line();

    fprintf(output, "##for loop break\njmp _%08d\n"
            "_%08d:\n", incl_to, break_to);

    // restore break and continue
    loop_to_inner = loop_to_prev;
    break_to_inner = break_to_prev;
    return;
}

void case_default()
{
    int false_branch = new_label();
    int next_case = new_label();
    int next_old = next_case_inner;
    next_case_inner = next_case;

    if (see("case"))
    {
        next();
        expr(0);
        fprintf(output, "\tcmp eax, ebx\n"
                        "\tjne _%08d\n", false_branch);
        if (next_old != 0)
            fprintf(output, "_%08d:\n", next_old);
        match(":");

        while (!see("case") && !see("default") && !see("}"))
        {
            line();
        }
        fprintf(output, "\tjmp _%08d\n", next_case);
    }
    else if (see("default"))
    {
        fprintf(output, "#default expr\n");
        next();
        if (next_old != 0)
        {
            fprintf(output, "_%08d:\n", next_old);
            next_case_inner = 0;
        }
        match(":");

        do
        {
            line();
        }
        while (!see("case") && !see("default") && !see("}"));
    }
    fprintf(output, "_%08d:\n", false_branch);
}

void switch_label()
{
    int break_to = new_label();
    int break_to_prev = break_to_inner;
    break_to_inner = break_to;

    match("switch");
    match("(");
    expr(0);
    fprintf(output, "#switch expr\n"
            "\tmov ebx, eax\n");
    match(")");
    match("{");

    do
    {
        case_default();
    }
    while ((see("case") || see("default")) && !feof(input));

    match("}");

    if (next_case_inner != 0)
    {
        fprintf(output, "_%08d:\n", next_case_inner);
        next_case_inner = 0;
    }

    fprintf(output, "_%08d:\n", break_to);

    break_to_inner = break_to_prev;
}

void assembler()
{
    match("asm");
    match("(");

    buffer[strlen(buffer) - 1] = 0;
    fprintf(output, buffer + 1);
    fprintf(output, "\n");
    next();

    match(")");
    match(";");
}

int _sizeof()
{
    int size;
    int local;
    int global;
    int strct;

    match("sizeof");
    match("(");

    //next();

    local = sym_lookup(locals, local_no, buffer);
    if (local >= 0)
    {
        size = locals_inst[local] * (((locals_type[local] & _ptr) != 0) ? ptr_size : locals_size[local]);
    }
    else
    {
        global = sym_lookup(globals, global_no, buffer);
        if (global >= 0)
        {
            size = globals_inst[global] * (((globals_type[global] & _ptr) != 0) ? ptr_size : globals_size[global]);
        }
        else
        {
            strct = sym_lookup(structs, struct_no, buffer);
            if (strct >= 0)
            {
                size = struct_size[strct];
            }
            else
            {
                size = see("char") ? 1 : see("short") ? 2 : see("int") ? 4 : see("long") ? 8 : see("bool") ? 4 : ptr_size;
            }
        }
    }

    while (!see(")"))
        next();
    //match(")");

    return size;
}

void branch(bool isexpr)
{
    int false_branch = new_label();
    int join = new_label();

    fprintf(output, "\tcmp eax, 0\n"
                    "\tje _%08d\n", false_branch);

    isexpr ? expr(1) : line();

    fprintf(output, "\tjmp _%08d\n", join);
    fprintf(output, "_%08d:\n", false_branch);

    if (isexpr)
    {
        match(":");
        expr(1);
    }
    else if (try_match("else"))
        line();

    fprintf(output, "_%08d:\n", join);
}

void if_branch()
{
    match("if");
    match("(");
    expr(0);
    match(")");
    branch(false);
}

void loop_break()
{
    match("break");
    fprintf(output, "\tjmp _%08d\n", break_to_inner);
    //match(";");
}

void loop_continue()
{
    match("continue");
    fprintf(output, "\tjmp _%08d\n", loop_to_inner);
    //match(";");
}

void while_loop()
{
    int loop_to = new_label();
    int break_to = new_label();
    int loop_to_prev = loop_to_inner;
    int break_to_prev = break_to_inner;

    loop_to_inner = loop_to;
    break_to_inner = break_to;

    fprintf(output, "_%08d:\n", loop_to);

    bool do_while = try_match("do");

    if (do_while)
        line();

    match("while");
    match("(");
    expr(0);
    match(")");

    fprintf(output, "\tcmp eax, 0\n"
                    "\tje _%08d\n", break_to);

    if (do_while)
        match(";");

    else
        line();

    fprintf(output, "\tjmp _%08d\n", loop_to);
    fprintf(output, "_%08d:\n", break_to);

    loop_to_inner = loop_to_prev;
    break_to_inner = break_to_prev;

}

//See decl() implementation
enum {decl_module = 1, decl_local, decl_param};

void line()
{
    if (see("if"))
        if_branch();

    else if (see("while") || see("do"))
        while_loop();

    else if (see("for"))
        for_loop();

    else if (see("break"))
        loop_break();

    else if (see("continue"))
        loop_continue();

    else if (see("switch"))
        switch_label();

    else if (see("sizeof"))
        _sizeof();

    else if (see("asm"))
        assembler();

    else if (see("int") || see("short") || see("char") || see("long") || see("bool") || see("struct") || see("union") || see("enum"))
    {
        decl(decl_local);
    }
    else if (try_match("{"))
    {
        while (waiting_for("}"))
            line();

        match("}");
    }
    else
    {
        bool ret = try_match("return");

        if (waiting_for(";"))
            expr(0);

        if (ret)
            fprintf(output, "\tjmp _%08d\n", return_to);

        match(";");
    }
}

void function(char * ident)
{
    //Prologue

    fprintf(output, ".globl _%s\n", ident);
    fprintf(output, "_%s:\n", ident);

    fputs("\tpush ebp\n"
          "\tmov ebp, esp\n", output);

    //Body

    return_to = new_label();

    line();

    //Epilogue

    fprintf(output, "_%08d:\n", return_to);
    fputs("\tmov esp, ebp\n"
          "\tpop ebp\n"
          "\tret\n", output);
}

void set_enum()
{
    int vz = 1;
    enum_count = 0;

    while (!try_match("{"))
        next();

    do
    {
        enum_name[enum_no] = strdup(buffer);
        next();
        if (try_match("="))
        {
            if (see("-"))
            {
                vz = -1;
                next();
            }
            enum_count = atoi(buffer) * vz;
            next();
        }
        enum_values[enum_no++] = enum_count++;
    }
    while (try_match(","));

    match("}");
    match(";");
}

void set_struct(int kind)
{
    int offset = 0;
    int size = 0;
    int ptr = 0;
    int struct_idx;
    int i;

    struct_size[struct_no] = 0;
    struct_start[struct_no] = struct_el_no;

    int is_struct = see("struct") ? 1 : 0;
    next();
    struct_nm = strdup(buffer);
    next();
    if (see("{"))
    {
        struct_idx = sym_lookup(structs, struct_no, struct_nm);
        require(struct_idx < 0, "struct name %s already used\n");
        next();
        do
        {
            structs[struct_no] = struct_nm;
            struct_el_offset[struct_el_no] = offset * is_struct;
            struct_el_type[struct_el_no] = see("int") ? _int : see("char") ? _char : see("short") ? _short : see("bool") ? _bool : 4;
            size = see("int") ? 4 : see("char") ? 1 : see("short") ? 2 : see("bool") ? 4 : 4;

            while (true)
            {
                next();
                if (see("*"))
                {
                    ptr = ptr << 1 | _ptr;
                    continue;
                }
                break;
            }
            if (ptr != 0)
            {
                size = ptr_size;
                struct_el_type[struct_el_no] = struct_el_type[struct_el_no] | ptr;
            }
            struct_el_size[struct_el_no] = size;
            struct_el_name[struct_el_no] = strdup(buffer);
            offset = offset + size;
            next();
            see(";");
            struct_el_no++;
            next();
        }
        while (!try_match("}"));
        struct_size[struct_no] = offset;
        struct_ende[struct_no] = struct_el_no;
//        int j;
//        puts(" ");
//        for(j=struct_start[struct_no];j<struct_ende[struct_no];j++)
//        {
//            printf("%s\t%d\n", struct_el_name[j], struct_el_offset[j]);
//        }
//        printf("size\t%d\n", offset);
        struct_no++;
    }

    if (((token == token_ident) || (token == token_other)) && (!see(";")))
    {
        int loc = sym_lookup(locals,  local_no,  buffer);
        int glo = sym_lookup(globals, global_no, buffer);
        int str = sym_lookup(structs, struct_no, struct_nm);

        if (kind == decl_local)
        {
            if (loc < 0)
            {
                ptr = 0;
                while (see("*"))
                {
                    ptr = (ptr << 1) | _ptr;
                    next();
                }
                cursize = struct_size[str];
                curtype = _struct | ptr;

                locals_struct[local_no] = str;
                locals_inst[local_no] = 1;

                new_local(strdup(buffer));

                if (curtype & _ptr)
                {
                    fprintf(output, "\tsub esp, %d\n", ptr_size);
                    locals_size[local_no-1] = ptr_size;
                }
                else
                {
                    fprintf(output, "\tsub esp, %d\n", cursize);
                    locals_size[local_no-1] = cursize;
                }
            }
        }
        else if (kind == decl_module)
        {
            if (glo < 0)
            {
                char *type;
                ptr = 0;
                while (see("*"))
                {
                    ptr = (ptr << 1) | _ptr;
                    next();
                }
                cursize = struct_size[str];
                curtype = _struct | ptr;

                globals_struct[global_no] = str;
                globals_size[global_no] = cursize;
                globals_inst[global_no] = 1;

                new_global(strdup(buffer));

                fprintf(output, ".section .data\n"
                                "_%s:\n", buffer);

                if (curtype & _ptr)
                {
                    fprintf(output, "\t.long 0 # %04x %d\n", type, curtype, cursize);
                }
                else
                {
                    for (i = struct_start[str]; i < struct_ende[str]; i++)
                    {
                        int t = struct_el_type[i];
                        if ((t & 0xf000) != 0)
                            type = "long";
                        else if ((t & 0x0fff) == _char)
                            type = "byte";
                        else if ((t & 0x0fff) == _short)
                            type = "word";
                        else if ((t & 0x0fff) == _long)
                            type = "qword";
                        else
                            type = "long";

                        fprintf(output, "\t.%s 0 # %04x %2d %s\n", type, struct_el_type[i], struct_el_offset[i], struct_el_name[i]);
                    }
                }
                fprintf(output, ".section .text\n");
            }
        }
        next();
    }

    match(";");
}

void decl(int kind)
{
    // A C declaration comes in three forms:
    // - Local decls, which end in a semicolon and can have an initializer.
    // - Parameter decls, which do not and cannot.
    // - Module decls, which end in a semicolon unless there is a function body.

    bool fn = false;
    bool fn_impl = false;
    int local;
    int funtype = -1;
    int funsize = -1;

    if (see("enum"))
    {
        set_enum();
        return;
    }
    else if ((see("struct")) || (see("union")))
    {
        set_struct(kind);
        return;
    }

    curtype = see("char") ? _char : see("int") ? _int : see("short") ? _short : see("long") ? _long : see("unsigned") ? _unsigned : see("bool") ? _bool : 0;
    cursize = see("char") ? 1 : see("short") ? 2 : see("int") ? 4 : see("long") ? 8 : see("unsigned") ? 4 : see("bool") ? 4 : word_size;
    token = see("char") ? token_char : see("short") ? token_short : see("int") ? token_int : see("long") ? token_long : token_other;

    next();

    do
    {
        int __ptr = 0;

        while (try_match("*"))
            __ptr = (__ptr << 1) + _ptr;

        curtype = curtype | __ptr;

        char * ident = strdup(buffer);

        // printf("decl(%d): %08X %s\n\n",__LINE__,curtype,ident); // Debug - Delete

        next();

        //BookMark [{Functions}]
        // Functions
        if (try_match("("))
        {
            if (kind == decl_module)
            {
                funtype = curtype;
                funsize = cursize;
                new_scope();
            }

            // Params
            //BookMark [{Params}]
            if (waiting_for(")"))
                do
                {
                    decl(decl_param);
                }
                while (try_match(","));

            match(")");

            curtype = funtype;
            cursize = funsize;

            new_fn(ident);
            fn = true;

            // Body
            //BookMark [{Body}]
            if (see("{"))
            {
                require(kind == decl_module, "a function implementation is illegal here\n");

                fn_impl = true;
                function(ident);
            }

            // Add it to the symbol table
            //BookMark [{Add to Symbol-Table}]
        }
        else
        {
            if (kind == decl_local)
            {
                int stack_bdf = 4;
                local = new_local(ident);
                locals_inst[local_no - 1] = 1;

                if (see("["))
                {
                    next();
                    locals_inst[local_no - 1] = atoi(buffer);
                    locals_type[local_no - 1] = locals_type[local_no - 1] | _array;
                    stack_bdf = locals_inst[local_no - 1] * cursize;
                    // locals_type[local_no-1] = locals_type[local_no-1] | _ptr;
                    + fprintf(output, "\tsub esp, %d\n", stack_bdf);
                    next();
                    match("]");
                }
                else if (curtype == _long)
                {
                    fprintf(output, "\tsub esp, %d\n", cursize);
                }
                else
                {
                    fprintf(output, "\tsub esp, %d\n", word_size);
                }
                local_offset = local_offset - stack_bdf;
                offsets[local_no - 1] = local_offset;
            }
            else
                (kind == decl_module ? new_global : new_param)(ident);
        }

        //Initialization
        //BookMark [{Initialization}]
        if (see("="))
        {
            require(!fn && kind != decl_param,
                    fn ? "cannot initialize a function\n" : "cannot initialize a parameter\n");
        }

        if (kind == decl_module)
        {
            globals_inst[global_no - 1] = 1;

            if (!fn)
                fputs(".section .data\n", output);

            if (try_match("="))
            {
                if (token == token_int)
                    if (curtype == _long)
                        fprintf(output, "_%s: .quad %s\n", ident, buffer);
                    else
                        fprintf(output, "_%s: .long %s\n", ident, buffer);

                else
                    error("expected a constant expression, found '%s'\n");

                next();

                //Static data defaults to zero if no initializer
            }
            else if (try_match("["))
            {
                char *gtype;
                int xtimes;

                if (token == token_int)
                {
                    globals_inst[global_no - 1] = atoi(buffer);
                    globals_type[global_no - 1] = globals_type[global_no - 1] | _array;
                    // globals_type[global_no-1] = globals_type[global_no-1] | _ptr;

                    gtype = (curtype == _char)? "byte" : (curtype == _short) ? "word" : (curtype == _long) ? "qword" : "long";
                    xtimes = atoi(buffer);

                    require(xtimes >= 0, "only positive values areallowed\n");

                    fprintf(output, "_%s:\n", ident);

                    next();
                    match("]");

                    if (try_match("="))
                    {
                        if (try_match("{"))
                        {
                            do
                            {
                               fprintf(output, "\t.%s %s\n", gtype, buffer);
                               xtimes--;
                               next();
                               if (see("}"))
                               {
                                   next();
                                   break;
                               }
                               require(xtimes>=0, "to many items\n");
                            } while (try_match(","));
                        }
                    }

                    if (xtimes>0)
                    {
                        fprintf(output,
                                "\t.rept %d\n"
                                "\t.%s 0\n"
                                "\t.endr\n", xtimes, gtype);
                    }
                }
                else
                    error("expected a constant expression, found '%s'\n");


            }
            else if (!fn)
                if (curtype == _long)
                    fprintf(output, "_%s: .quad 0\n", ident);
                else
                    fprintf(output, "_%s: .long 0\n", ident);

            if (!fn)
                fputs(".section .text\n", output);

        }
        else if (try_match("="))
        {
            expr(0);
            fprintf(output, "\tmov dword ptr [ebp%+d], eax\t# %s\n", offsets[local], locals[local]);
        }

        if (kind == decl_param)
            break;

    }
    while (try_match(","));

    if (!fn_impl && kind != decl_param)
        match(";");
}

void program()
{
    read_line();
    next_char();
    next();

    errors = 0;

    while (!feof(input))
        decl(decl_module);
}

void do_pragma()
{
    next();

    if (see("list"))
    {
        next();
        match("(");

        if (see("on"))
            list = 1;
        if (see("off"))
            list = 0;

        next();

        if (!see(")"))
        {
            fprintf(stderr, "%s:%d: error: ", inputname, curln);
            fprintf(stderr, "expected ')', found '%s'\n", buffer);
            errors++;
        }
    }
    else if (see("source"))
    {
        next();
        match("(");

        if (see("on"))
            source = 1;
        if (see("off"))
            source = 0;

        next();

        if (!see(")"))
        {
            fprintf(stderr, "%s:%d: error: ", inputname, curln);
            fprintf(stderr, "expected ')', found '%s'\n", buffer);
            errors++;
        }
    }
}

void do_include()
{
    int i;
    char *include;

    next_char();
    buflength = 0;

    if (curch == '<')
    {
        next_char();
        for (i = 0; curch != '>'; i++)
        {
            if (curch == '\n')
                return;
            (buffer + i)[0] = curch;
            next_char();
        }
        (buffer + i)[0] = 0;
        while (curch != '\n')
            next_char();
        include = malloc(strlen(home) + strlen(buffer) + 10);
        strcpy(include, home);
        strcat(include, "include\\");
        strcat(include, buffer);
        // puts(include);
        old_input = input;
        old_line = curln;
        old_iname = inputname;
        input = fopen(include, "r");
        if (input == 0)
        {
            input = old_input;
            buflength = 0;
            error("Header file not found\n");
            return;
        }
        curln = 0;
        inputname = strdup(buffer);
        tc = '+';
    }
    else if (curch == '"')
    {
        next_char();
        for (i = 0; curch != '"'; i++)
        {
            if (curch == '\n')
                return;
            (buffer + i)[0] = curch;
            next_char();
        }
        (buffer + i)[0] = 0;
        while (curch != '\n')
            next_char();
        include = malloc(strlen(home) + strlen(buffer) + 1);
        strcpy(include, home);
        strcat(include, buffer);
        // puts(include);
        old_input = input;
        old_line = curln;
        old_iname = inputname;
        input = fopen(include, "r");
        if (input == 0)
        {
            input = old_input;
            buflength = 0;
            error("Header file not found\n");
            return;
        }
        curln = 0;
        inputname = strdup(buffer);
        tc = '*';
    }
    else
    {
        error("Syntax error '#include'\n");
    }

    return;
}

void do_preprocess()
{
    next_char();
    buflength = 0;

    while (isalpha(curch))
        eat_char();
    buffer[buflength] = 0;

    if (see("include"))
        do_include();

    if (see("pragma"))
        do_pragma();
}

int main(int argc, char ** argv)
{
    char *version = "mini-c v0.18a";
    // char *fn_out;
    int i;

    if (argc != 2)
    {
        printf("%s\nUsage: cc <file>", version);
        return 1;
    }

    home = malloc(strlen(argv[0]) + 200);
    if (argv[0][1] == ':')
    {
        strcpy(home, argv[0]);
    }
    else
    {
        GetCurrentDirectory(strlen(argv[0]) + 200, home);
        strcat(home, "\\");
        strcat(home, argv[0]);
    }

    for (i = strlen(home); i > 0; i--)
    {
        if (home[i] == '\\')
        {
            home[i + 1] = 0;
            break;
        }
    }

    outputname = strdup(argv[1]);
    outputname[strlen(outputname) - 1] = 's';

    output = fopen(outputname, "w");
    printf("%s\n%s  %s\n\n", version, __DATE__, __TIME__);

    lex_init(argv[1], 10240);

    sym_init(10240);

    /*  //No arrays? Fine! A 0xFFFFFF terminated string of null terminated strings will do.
    //A negative-terminated null-terminated strings string, if you will
    char * std_fns = "malloc\0calloc\0free\0atoi\0fopen\0fclose\0fgetc\0ungetc\0feof\0fputs\0fprintf\0puts\0printf\0"
    "isalpha\0isdigit\0isalnum\0strlen\0strcmp\0strchr\0strcpy\0strdup\0\xFF\xFF\xFF\xFF";

    //Remember that mini-c is typeless, so this is both a byte read and a 4 byte read.
    //(char) 0xFF == -1, (int) 0xFFFFFF == -1
    while (std_fns[0] != -1)
    {
    new_fn(std_fns);
    std_fns = std_fns + strlen(std_fns) + 1;
    }
     */
    bjct = "dword";
    rgst = "eax";

    fprintf(output,
            "# %s\n"
            "# file: %s\n"
            ".intel_syntax noprefix\n\n", version, inputname);

    program();

    fclose(output);

    printf("\n<>Errors: %d\n", errors);

    lex_end();
    sym_end();

    return errors != 0;
}
