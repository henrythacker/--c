#include <stdio.h>
#include <ctype.h>
#include "nodes.h"
#include "C.tab.h"
#include <string.h>
#include "interpreter.h"
#include "tacgenerator.h"
#include "mips.h"

#define MODE_PARSE 0
#define MODE_TAC_GEN 1
#define MODE_INTERPRET 2
#define MODE_MIPS 3

char *named(int t)
{
    static char b[100];
    if (isgraph(t) || t==' ') {
      sprintf(b, "%c", t);
      return b;
    }
    switch (t) {
      default: return "???";
    case IDENTIFIER:
      return "id";
    case CONSTANT:
      return "constant";
    case STRING_LITERAL:
      return "string";
    case LE_OP:
      return "<=";
    case GE_OP:
      return ">=";
    case EQ_OP:
      return "==";
    case NE_OP:
      return "!=";
    case EXTERN:
      return "extern";
    case AUTO:
      return "auto";
    case INT:
      return "int";
    case VOID:
      return "void";
    case APPLY:
      return "apply";
    case LEAF:
      return "leaf";
    case IF:
      return "if";
    case ELSE:
      return "else";
    case WHILE:
      return "while";
    case CONTINUE:
      return "continue";
    case BREAK:
      return "break";
    case RETURN:
      return "return";
    }
}

void print_leaf(NODE *tree, int level)
{
    TOKEN *t = (TOKEN *)tree;
    int i;
    for (i=0; i<level; i++) putchar(' ');
    if (t->type == CONSTANT) printf("%d\n", t->value);
    else if (t->type == STRING_LITERAL) printf("\"%s\"\n", t->lexeme);
    else if (t) puts(t->lexeme);
}

void print_tree0(NODE *tree, int level)
{
    int i;
    if (tree==NULL) return;
    if (tree->type==LEAF) {
      print_leaf(tree->left, level);
    }
    else {
      for(i=0; i<level; i++) putchar(' ');
      printf("%s\n", named(tree->type));
/*       if (tree->type=='~') { */
/*         for(i=0; i<level+2; i++) putchar(' '); */
/*         printf("%p\n", tree->left); */
/*       } */
/*       else */
        print_tree0(tree->left, level+2);
      print_tree0(tree->right, level+2);
    }
}

void print_tree(NODE *tree)
{
    print_tree0(tree, 0);
}

extern int yydebug;
extern NODE* yyparse(void);
extern NODE* ans;
extern void init_symbtable(void);

char *mode_to_string(int mode) 
{
	switch(mode) 
	{
		case(MODE_PARSE):
			return "parse tree";
		case(MODE_INTERPRET):
			return "interpret";
		case(MODE_TAC_GEN):
			return "three address code generator";						
		case(MODE_MIPS):
			return "MIPS code generator";			
		default:
			return "Unknown";
	}
}

/* Start processing the input per the relevant mode */
void process(NODE *tree, int run_mode)
{
	switch(run_mode) 
	{
		case(MODE_PARSE):
			printf("\nparse finished with %p\n", tree);
	    	print_tree(tree);
			break;
		case(MODE_INTERPRET):
			start_interpret(tree);
			break;
		case(MODE_TAC_GEN):
			print_tac(start_tac_gen(tree));
			break;
		case(MODE_MIPS):
			code_gen(tree);
			break;
		default:
			return;
	}	
}

/* Print out usage instructions */
void print_runflags() {
	printf("--C Compiler - Input syntax\n");
	printf("\n");
	printf("./mycc [-p|-i|-t|-m] < input_source\n");
	printf("\n");	
	printf("-p = parse mode - print out the parse tree and terminate\n");
	printf("-i = interpret - interpret the parse tree and print the result\n");
	printf("-t = TAC generator - print out TAC representation of the programme\n");
	printf("-m = MIPS generator - generate MIPS machine code\n");
	printf("\n");
}

int main(int argc, char** argv)
{
    NODE* tree;
	int mode = MODE_PARSE;
    if (argc==1) 
	{
		/* if no flag specified - print out flags */
		print_runflags();
	}
	else
	{
		/* Define the allowed parameters */
		if (strcmp(argv[1],"-p")==0) mode=MODE_PARSE;				
		if (strcmp(argv[1],"-i")==0) mode=MODE_INTERPRET;		
		if (strcmp(argv[1],"-t")==0) mode=MODE_TAC_GEN;		
		if (strcmp(argv[1],"-m")==0) mode=MODE_MIPS;		
		init_symbtable();
	    yyparse();
	    tree = ans;
		printf("\n-------\n");
		process(tree, mode);
	}
    return 0;
}
