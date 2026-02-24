#ifndef CALC_H
#define CALC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//defs/data

#define MAX_VARS 100

typedef struct var {
	char *name;
	int value;
	struct var *next;
} var;

typedef struct env {
	struct var *vars;
	struct env *parent;
} env;

typedef enum expr_type {
	EXPR_LITERAL,
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_VARIABLE,
	EXPR_ASSIGN,
	EXPR_PRINT,
	EXPR_IF,
	EXPR_SEQUENCE,
	EXPR_BLOCK,
	EXPR_LET,
	EXPR_WHILE
} expr_type;

typedef enum operator_type {
	OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_PWR, OP_NEG,
	OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,
	OP_AND, OP_OR
} operator_type;

typedef struct expression {
	expr_type type;

	union {
		int value; //if literal
				   
		char *var_name; //for var ref

		struct {
			struct expression *operand;
			operator_type operation;
		} unary;

		struct {
			struct expression *left;
			struct expression *right;
			operator_type operation;
		} binary;

		struct {
			struct expression *value;
			char *var_name;
		} assign;
		
		struct {
			struct expression *value;
		} print;

		struct {
			struct expression *condition;
			struct expression *then_branch;
			struct expression *else_branch;
		} if_expr;

		struct {
			struct expression *left;
			struct expression *right;
		} sequence;

		struct {
			struct expression *body;
		} block;

		struct {
			struct expression *value;
			char *var_name;
		} let;

		struct {
			struct expression *condition;
			struct expression *body;
		} while_expr;

	} data;

} expression;

typedef struct parser {
	const char *input;
	int pos;
} parser;

typedef struct variable {
	char name[32];
	int value;
} variable;

//operations

static int pwr(int a, int b);

//parser functions

char peek(parser *p);
char advance(parser *p);
void skip_ws(parser *p);

//environment functions

env *new_env(env *parent);
var *env_lookup(env *e, const char *name);
int env_assign(env *e, const char *name, int value);
void env_define(env *e, const char *name, int value);

//constuctors

expression *new_literal(int value);
expression *new_binary(operator_type op, expression *left, expression *right);
expression *new_unary(operator_type op, expression *operand);
expression *new_variable(char *name);
expression *new_assign(char *name, expression *value);
expression *new_print(expression *value);
expression *new_if(expression *cond, expression *then_branch, expression *else_branch);
expression *new_sequence(expression *left, expression *right);
expression *new_block(expression *body);
expression *new_let(char *name, expression *value);
expression *new_while(expression *condition, expression *body);

//identifiers

int is_identifier_start(char c);
int is_identifier_char(char c);

//evaluation

int evaluate_expr(expression *expr, env *e);

//free

void free_expr(expression *expr);
void free_env(env *e);

//parsing

expression *parse_number(parser *p);
char *parse_identifier(parser *p);
expression *parse_factor(parser *p);
expression *parse_unary(parser *p);
expression *parse_power(parser *p);
expression *parse_term(parser *p);
expression *parse_expression(parser *p);
expression *parse_statement(parser *p);
expression *parse_comparsion(parser *p);
expression *parse_additive(parser *p);
expression *parse_logical_and(parser *p);
expression *parse_logical_or(parser *p);
expression *parse_if(parser *p);
expression *parse_assignment(parser *p);
expression *parse_program(parser *p);

//file handling

void run_repl(void);
char *read_file(const char *filename);
void run_file(const char *filename);

#endif
