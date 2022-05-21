/*
 * bfc.c
 *   An optimizing bytecode compiler for BF
 *   -Adon Shapiro (2022)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPELEN 30000
#define SUCCESS 0
#define FAILURE 1

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_INC,
    OP_DEC,
    OP_PUT,
    OP_GET,
    OP_JMP,
    OP_JNE,
    NUM_OPS
} Opcode;

typedef struct {
    Opcode op;
    int arg;
} Instruction;

/*
 * Tokenizes the given input.
 * Can fail if the given input does not fit on the BF machine's "tape".
 */
static int lex(FILE *input, char *tokens) {
    int count = 0;
    char cur = getc(input);

    while (cur != EOF) {
        if (count >= TAPELEN - 2)
            return FAILURE;
        switch (cur) {
        case '+': case '-': case '>': case '<':
        case '.': case ',': case '[': case ']':
            tokens[count] = cur;
            count++;
        }
        cur = getc(input);
    }

    tokens[count] = '\0';
    return SUCCESS;
}

/*
 * Creates an abstract syntax tree from the given tokens.
 * Fails if a syntax error is encountered (e.g. unmatched square brackets).
 */
static int parse(const char *tokens, Opcode *ast) {
    int i;
    int unmatched = 0;

    for (i = 0; tokens[i] != '\0'; i++) {
        switch (tokens[i]) {
        case '+':
            ast[i] = OP_ADD;
            break;
        case '-':
            ast[i] = OP_SUB;
            break;
        case '>':
            ast[i] = OP_INC;
            break;
        case '<':
            ast[i] = OP_DEC;
            break;
        case '.':
            ast[i] = OP_PUT;
            break;
        case ',':
            ast[i] = OP_GET;
            break;
        case '[':
            unmatched++;
            ast[i] = OP_JMP;
            break;
        case ']':
            unmatched--;
            ast[i] = OP_JNE;
            break;
        }
    }

    if (unmatched != 0)
        return FAILURE;

    ast[i] = NUM_OPS;
    return SUCCESS;
}

/*
 * Compiles the abstract syntax tree into bytecode.
 */
static int compile(const Opcode *ast, Instruction *bytecode) {
    int i;

    for (i = 0; ast[i] != NUM_OPS; i++) {
        bytecode[i] = (Instruction) { ast[i], 1 };
    }

    bytecode[i] = (Instruction) { NUM_OPS, 0 };
    return SUCCESS;
}

/*
 * Performs optimizations on the given bytecode.
 */
static int optimize(const Instruction *bcode, Instruction *out) {
    /* Quick and dirty linked list stack for the jump indices */
    struct Node {
        int hd;
        struct Node *tl;
    };
    struct Node *loop_stack = NULL;
    int i;
    int cur = 0;

    for (i = 0; bcode[i].op != NUM_OPS; i++) {
        Instruction inst = { bcode[i].op, bcode[i].arg };
        
        /* Compute jump indices */
        if (inst.op == OP_JMP) {
            out[cur] = inst;
            struct Node *is = malloc(sizeof(struct Node));
            is->hd = cur;
            is->tl = loop_stack;
            loop_stack = is;
            cur++;
            continue;
        }
        if (inst.op == OP_JNE) {
            out[loop_stack->hd].arg = cur;
            inst.arg = loop_stack->hd;
            out[cur] = inst;
            struct Node *is = loop_stack->tl;
            free(loop_stack);
            loop_stack = is;
            cur++;
            continue;
        }

        /* Fold multiple simple instructions into one compound instruction */
        for (int j = i + 1; bcode[j].op == inst.op; j++) {
            inst.arg++;
            i = j;
        }
        out[cur] = inst;
        cur++;
    }

    out[cur] = (Instruction) { NUM_OPS, 0 };
    return SUCCESS;
}

/*
 * Runs optimized bytecode. This is the BF machine.
 */
static void run(const Instruction *program) {
    int ip = 0;
    int dp = 0;
    char data[TAPELEN];
    for (int i = 0; i < TAPELEN; i++)
        data[i] = 0;

    while (program[ip].op != NUM_OPS) {
        switch (program[ip].op) {
        case OP_ADD:
            data[dp] = data[dp] + program[ip].arg;
            break;
        case OP_SUB:
            data[dp] = data[dp] - program[ip].arg;
            break;
        case OP_INC:
            dp = dp + program[ip].arg;
            break;
        case OP_DEC:
            dp = dp - program[ip].arg;
            break;
        case OP_PUT:
            for (int i = 0; i < program[ip].arg; i++)
                putchar(data[dp]);
            break;
        case OP_GET:
            for (int i = 0; i < program[ip].arg; i++)
                data[dp] = getchar();
            break;
        case OP_JMP:
            if (data[dp] == 0)
                ip = program[ip].arg;
            break;
        case OP_JNE:
            if (data[dp] != 0)
                ip = program[ip].arg;
            break;
        default:
            break;
        }
        ip++;
    }
}

/*
 * Exit with error.
 */
static void quit(const char *msg) {
    fputs(msg, stderr);
    exit(1);
}

/*
 * Entry point. Program is read from stdin.
 */
int main(void) {
    int stat;
    char tokens[TAPELEN];
    Opcode ast[TAPELEN];
    Instruction bytecode[TAPELEN];
    Instruction optimized[TAPELEN];

    stat = lex(stdin, tokens);
    if (stat != SUCCESS)
        quit("ERROR: Program exceeds available memory\n");
    stat = parse(tokens, ast);
    if (stat != SUCCESS)
        quit("SYNTAX ERROR: Unmatched '[' or ']'\n");
    stat = compile(ast, bytecode);
    if (stat != SUCCESS)
        quit("ERROR: Compilation failed");
    stat = optimize(bytecode, optimized);
    if (stat != SUCCESS)
        quit("ERROR: Optimization failed");

    clearerr(stdin);
    run(optimized);
    return 0;
}
