//-------------------------------//
// mini-c, by Sam Nipps (c) 2015 //
// MIT license                   //
//-------------------------------//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

//No enums :(
int ptr_size = 4;
int word_size = 4;

char * outputname;
FILE * output;

char * inputname;
FILE * input;

//==== Lexer ====
int curln;
char curch;
char *line_cache;
char *line_pointer;

char * buffer;
int buflength;
int token;

int token_other = 0;
int token_ident = 1;
int token_int = 2;
int token_char = 3;
int token_str = 4;


void println()
{
    printf("%5d: ", curln);
}

void read_line()
{
    line_pointer = line_cache;
    do
    {
        if (feof(input)) break;
        line_pointer[0] = fgetc(input);
    } while (((line_pointer++)[0] != '\n') && !(feof(input)));

//    if (*(line_pointer-1) != '\n') 
    if (feof(input))
        (line_pointer-1)[0] = '\n';
    line_pointer[0] = 0;
    fprintf(output, "# %s\n", line_cache);
    line_pointer = line_cache;
}

char next_char()
{
    if (curch == '\n')
    {
        curln++;
        println();
        read_line();
    }

    curch = line_pointer[0];
    line_pointer++;
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
    if (curch == '#'
    || (curch == '/' && (next_char() == '/' || prev_char('/'))))
    {
        while (curch != '\n' && !feof(input))
            next_char();

            //Restart the function (to skip subsequent whitespace, comments and pp)
        next();
        return;
    }
    else if (curch == '/' && (next_char() == '*' || prev_char('*')))
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
    if (isalpha(curch))
    {
        token = token_ident;

        while ((isalnum(curch) || curch == '_') && !feof(input))
            eat_char();

            //Integer literal
    }
    else if (curch == '0')
    {
        token = token_int;
        eat_char();
        if (curch=='x' || curch=='X')
        {
            while ((((curch>='0')&&(curch<='9')) ||
                    ((curch>='A')&&(curch>='F')) ||
                    ((curch>='a')&&(curch>='f'))) && !feof(input))
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
    else if (curch == '+' || curch == '-' || curch == '=' || curch == '|' || curch == '&')
    {
        eat_char();

        if (curch == buffer[0])
            eat_char();

            //Operators which may be followed by a '='
    }
    else if (curch == '!' || curch == '>' || curch == '<')
    {
        eat_char();

        if (curch == '=')
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

void error(char * format)
{
    printf("\n%s:%d: error: ", inputname, curln);
    //Accepting an untrusted format string? Naughty!
    printf(format, buffer);
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
        printf("%s:%d: error: ", inputname, curln);
        printf("expected '%s', found '%s'\n", look, buffer);
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

char ** globals;
int global_no;
bool * is_fn;
int *used_fn;
int use_fn;

char ** locals;
int local_no;
int param_no;
int * offsets;

char ** enum_names;
int * enum_values;
int enum_no;
int enum_count;

void sym_init(int max)
{
    globals = malloc(ptr_size * max);
    global_no = 0;
    is_fn = calloc(max, ptr_size);
    used_fn =malloc(word_size * max);
    use_fn = 0;

    locals = malloc(ptr_size * max);
    local_no = 0;
    param_no = 0;
    offsets = calloc(max, word_size);

    enum_names = malloc(ptr_size * max);
    enum_values = calloc(max, ptr_size);
    enum_no = 0;
}

void table_end(char ** table, int table_size)
{
    int i = 0;

    while (i < table_size)
        free(table[i++]);
}

void sym_end()
{
    //    table_end(globals, global_no);
    free(globals);
    free(is_fn);

    table_end(locals, local_no);
    free(locals);
    free(offsets);
}

void new_global(char * ident)
{
    globals[global_no++] = ident;
}

void new_fn(char * ident)
{
    is_fn[global_no] = true;
    new_global(ident);
}

int new_local(char * ident)
{
    int var_index = local_no - param_no;

    locals[local_no] = ident;
    //The first local variable is directly below the base pointer
    offsets[local_no] = -word_size * (var_index + 1);
    return local_no++;
}

void new_param(char * ident)
{
    int local = new_local(ident);

    //At and above the base pointer, in order, are:
    // 1. the old base pointer, [ebp]
    // 2. the return address, [ebp+W]
    // 3. the first parameter, [ebp+2W]
    //   and so on
    offsets[local] = word_size * (2 + param_no++);
}

//Enter the scope of a new function
void new_scope()
{
    table_end(locals, local_no);
    local_no = 0;
    param_no = 0;
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
    else if (token == token_ident)
    {
        int global = sym_lookup(globals, global_no, buffer);
        int local = sym_lookup(locals, local_no, buffer);
        int enumidx = sym_lookup(enum_names,enum_no, buffer);

        require(global >= 0 || local >= 0 || enumidx >= 0, "no symbol '%s' declared\n");
        next();

        if (see("=") || see("++") || see("--"))
            lvalue = true;

        if (enumidx >= 0)
        {
            fprintf(output, "\tmov eax, %d""\t# %s\n", enum_values[enumidx], enum_names[enumidx]);
        }
        else if (global >= 0) 
        {
            if (!is_fn[global])
            {
//              fprintf(output, "\t%s eax, [_%s]\n", is_fn[global] || lvalue ? "lea" : "mov", globals[global]);
                fprintf(output, "\t%s eax, [_%s]\n", lvalue ? "lea" : "mov", globals[global]);
            }
            else
            {
                used_fn[use_fn++] = global;
            }
        }
        else if (local >= 0)
            fprintf(output, "\t%s eax, [ebp%+d]\n", lvalue ? "lea" : "mov", offsets[local]);

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
    factor();

    while (true)
    {
        if (try_match("("))
        {
//            fputs("\tpush eax\n", output);

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
                } while (try_match(","));

                fprintf(output, "_%08d:\n", start_label);
                fprintf(output, "\tjmp _%08d\n", prev_label);
                fprintf(output, "_%08d:\n", end_label);
            }

            match(")");

            use_fn--;
//            fprintf(output, "\tcall dword ptr [esp+%d]; _%s\n", arg_no * word_size,globals[used_fn[use_fn]]);
//            fprintf(output, "\tadd esp, %d\n", (arg_no + 1) * word_size);
            fprintf(output, "\tcall _%s\n",globals[used_fn[use_fn]]);
            fprintf(output, "\tadd esp, %d\n", (arg_no) * word_size);

        }
        else if (try_match("["))
        {
            fputs("\tpush eax\n", output);

            expr(0);
            match("]");

            if (see("=") || see("++") || see("--"))
                lvalue = true;

            fprintf(output, "\tpop ebx\n"
            "\t%s eax, [eax*%d+ebx]\n", lvalue ? "lea" : "mov", word_size);

        }
        else
            return;
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
    else
    {
        //This function call compiles itself
        object();

        if (see("++") || see("--"))
        {
            fprintf(output, "\tmov ebx, eax\n"
            "\tmov eax, [ebx]\n"
            "\t%s dword ptr [ebx], 1\n", see("++") ? "add" : "sub");

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
    : level == 4 ? see("+") || see("-") || see("|") || see("&")
    : level == 3 ? see("==") || see("!=") || see("<") || see(">") || see("<=") || see(">=")
    : false)
    {
        if (see("/")) div = 1;
        if (see("%")) div = 2;

        fputs("\tpush eax\n", output);

        char * instr = see("+") ? "add" : see("-") ? "sub" : see("|") ? "or" : see("&") ? "and" : see("*") ? "imul" : see("/") ? "idiv" : see("%") ? "idiv" :
        see("==") ? "e" : see("!=") ? "ne" : see("<") ? "l" : see(">") ? "g" : see("<=") ? "le" : "ge";

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
                "\txor edx,edx\n"
                "\t%s ebx\n", instr);
            }
            else
            {
                fprintf(output, "\tmov ebx, eax\n"
                "\tpop eax\n"
                "\txor edx,edx\n"
                "\t%s ebx\n"
                "\tmov eax,edx\n", instr);
            }
        }
        else if (level == 4)
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

        needs_lvalue("assignment requires a modifiable object\n");
        expr(level + 1);

        fputs("\tpop ebx\n"
        "\tmov dword ptr [ebx], eax\n", output);
    }
}

void line();

void for_loop()
{
    // labels for break and continue
    int loop_to = new_label();
    int break_to = new_label();
    int loop_to_prev = loop_to_inner;
    int break_to_prev = break_to_inner;
    int body_to = new_label();
    int incl_to = new_label();

    loop_to_inner = loop_to;
    break_to_inner = break_to;
    
    // for body intro
    match("for");
    match("(");
    if (!see(";"))
        do
        {
            expr(0);
        } while (try_match(","));

    match(";");

    // for body condition
    fprintf(output, //"# for loop entry\n"
                    "_%08d:\n", loop_to);
    
    if(!see(";"))
        expr(0);
    else
        fprintf(output,"\tmov eax, 1\n");

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
        } while (try_match(","));

    match(")");

    fprintf(output, //"# for loop body\n"
    "\tjmp _%08d\n"
    "_%08d:\n", loop_to, body_to);

    line();

    fprintf(output, "#for loop break\njmp _%08d\n"
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
            fprintf(output,"_%08d:\n", next_old);
        match(":");
        
        while (!see("case") && !see("default") && !see("}"))
        {
            line();
        } 
        fprintf(output,"\tjmp _%08d\n", next_case);
    }
    else if (see("default"))
    {
        fprintf(output, "#default expr\n");
        next();
        if (next_old != 0)
        {
            fprintf(output,"_%08d:\n", next_old);
            next_case_inner = 0;
        }
        match(":");

        do
        {
            line();
        } while (!see("case") && !see("default") && !see("}"));
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
    fprintf(output, //"#switch expr\n"
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
        fprintf(output,"_%08d:\n", next_case_inner);
        next_case_inner = 0;
    }

    fprintf(output, "_%08d:\n", break_to);

    break_to_inner = break_to_prev;
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

void decl(int kind);

//See decl() implementation
int decl_module = 1;
int decl_local = 2;
int decl_param = 3;

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

    else if (see("int") || see("char") || see("bool"))
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
        enum_names[enum_no] = strdup(buffer);
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
    } while (try_match(","));

    match("}");
    match(";");

}

void decl(int kind)
{
    //A C declaration comes in three forms:
    // - Local decls, which end in a semicolon and can have an initializer.
    // - Parameter decls, which do not and cannot.
    // - Module decls, which end in a semicolon unless there is a function body.

    bool fn = false;
    bool fn_impl = false;
    int local;

    if (see("enum"))
    {
        set_enum();
        return;
    }

    next();

    while (try_match("*"))
        ;

    char * ident = strdup(buffer);
    next();

    //Functions
    if (try_match("("))
    {
        if (kind == decl_module)
            new_scope();

            //Params
        if (waiting_for(")"))
            do
            {
                decl(decl_param);
            } while (try_match(","));

        match(")");

        new_fn(ident);
        fn = true;

        //Body
        if (see("{"))
        {
            require(kind == decl_module, "a function implementation is illegal here\n");

            fn_impl = true;
            function(ident);
        }

        //Add it to the symbol table
    }
    else
    {
        if (kind == decl_local)
        {
            local = new_local(ident);
            fprintf(output, "\tsub esp, %d\n", word_size);
        }
        else
            (kind == decl_module ? new_global : new_param)(ident);
    }

    //Initialization

    if (see("="))
        require(!fn && kind != decl_param,
        fn ? "cannot initialize a function\n" : "cannot initialize a parameter");

    if (kind == decl_module)
    {
        fputs(".section .data\n", output);

        if (try_match("="))
        {
            if (token == token_int)
                fprintf(output, "_%s: .quad %s\n", ident, buffer);

            else
                error("expected a constant expression, found '%s'\n");

            next();

            //Static data defaults to zero if no initializer
        }
        else if (!fn)
            fprintf(output, "_%s: .quad 0\n", ident);

        fputs(".section .text\n", output);

    }
    else if (try_match("="))
    {
        expr(0);
        fprintf(output, "\tmov dword ptr [ebp%+d], eax\n", offsets[local]);
    }

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

int main(int argc, char ** argv)
{
    char *fn_out;
    if (argc != 2)
    {
        puts("Usage: cc <file>");
        return 1;
    }

    outputname = strdup(argv[1]);
    outputname[strlen(outputname) - 1] = 's';

    output = fopen(outputname, "w");

    lex_init(argv[1], 256);

    sym_init(256);

    //No arrays? Fine! A 0xFFFFFF terminated string of null terminated strings will do.
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

    fprintf(output, "# mini-c v0.9.1\n"
                    "# %s\n"
                    ".intel_syntax noprefix\n\n", inputname);

    program();

    fclose(output);

    printf("\n<>Errors: %d\n", errors);

    lex_end();
    sym_end();

    return errors != 0;
}
