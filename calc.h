#ifndef CALC_H
#define CALC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//defs/data

#define MAX_VARS 100

#define COLOR_HEAD   "\033[1;32m"
#define COLOR_PROMPT "\033[1;34m"
#define COLOR_CONT   "\033[1;30m"
#define COLOR_RESET  "\033[0m"

typedef enum value_type {
	VAL_INT,
	VAL_FUNCTION,
	VAL_ARRAY,
	VAL_STRING
} value_type;

typedef struct function function;

typedef struct value {
	value_type type;
	union {
		int int_val;
		function *func_val;
		char *str_val;

		struct {
			int *data;
			int length;
		} array_val;

	};
} value;

typedef struct var {
	char *name;
	value val;
	struct var *next;
} var;

typedef struct env {
	struct var *vars;
	struct env *parent;
} env;

typedef value (*builtin_func)(int arg_count, value *args);

struct function {
	char **params;
	int param_count;
	struct expression *body;
	struct env *closure;
	builtin_func c_func;
};

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
	EXPR_WHILE,
	EXPR_FUNCTION,
	EXPR_CALL,
	EXPR_RETURN,
	EXPR_ARRAY_LITERAL,
	EXPR_ARRAY_ACCESS,
	EXPR_ARRAY_ASSIGN,
	EXPR_STRING_LITERAL
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

		struct {
			struct expression *body;
			char *name;
			char **params;
			int param_count;
		} function_expr;

		struct {
			struct expression *callee;
			struct expression **args;
			int arg_count;
		} call_expr;
			
		struct {
			struct expression *value;
		} return_expr;

		struct {
			struct expression **elements;
			int count;
		} array_literal;

		struct {
			struct expression *array;
			struct expression *index;
		} array_access;

		struct {
			struct expression *array;
			struct expression *index;
			struct expression *value;
		} array_assign;

		struct {
			char *str;
		} string_literal;

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

//builtin functions

value builtin_print(int arg_count, value *args);
value builtin_len(int arg_count, value *args);
value builtin_swap(int arg_count, value *args);
value builtin_copy(int arg_count, value *args);

//helper for repl

static void print_cont_prompt(int indent_level);

//parser functions

char peek(parser *p);
char advance(parser *p);
void skip_ws(parser *p);

//environment functions

env *new_env(env *parent);
var *env_lookup(env *e, const char *name);
int env_assign(env *e, const char *name, value val);
void env_define(env *e, const char *name, value val);

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
expression *new_function(char *name, char **params, int param_count, expression *body);
expression *new_call(expression *callee, expression **args, int arg_count);
expression *new_return(expression *value);
expression *new_arr_literal(expression **elements, int count);
expression *new_arr_access(expression *arr, expression *index);
expression *new_arr_assign(expression *arr, expression *index, expression *value_expr);
expression *new_string_literal(char *str);

//identifiers

int is_identifier_start(char c);
int is_identifier_char(char c);

//evaluation

value evaluate_expr(expression *expr, env *e);

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

//std function registration

static void print_cont_prompt(int indent_level);
static value reg_print(void);
static value reg_len(void);

//file handling

void run_repl(void);
char *read_file(const char *filename);
void run_file(const char *filename);

#endif
