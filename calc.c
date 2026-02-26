#include "calc.h"

/*******************************global**********************************/

static int return_flag = 0;
static value return_value;

/*****************************operations********************************/

//exp by squaring, O(log(n))
static int pwr(int a, int b) {
	if (b < 0) {
		fprintf(stderr, "negative exponent not supported for int\n");
		return -1;
	}
	if ((a == 0 && b == 0) || a == 1) {
		return 1;
	}
	int res = 1;
	while (b > 0) {
		if (b & 1) {
			res *= a;
		}
		a *= a;
		b >>= 1;
	}
	return res;
}

/**************************builtin function*****************************/

value builtin_print(int arg_count, value *args) {
	if (arg_count != 1 || args[0].type != VAL_INT) {
		fprintf(stderr, "print expects a single int arg\n");
		exit(1);
	}

	printf("%d\n", args[0].int_val);

	value result;
	result.type = VAL_INT;
	result.int_val = 0;
	return result;
}

/**************************parser functions*****************************/

char peek(parser *p) {
	return p->input[p->pos];
}

char advance(parser *p) {
	return p->input[p->pos++];
}

void skip_ws(parser *p) {
	while (isspace(peek(p))) {
		advance(p);
	}
}

int is_at_end(parser *p) {
	return p->input[p->pos] == '\0';
}

/************************************************************************/

env *new_env(env *parent) {
	env *e = malloc(sizeof(env));
	e->vars = NULL;
	e->parent = parent;
	return e;
}

var *env_lookup(env *e, const char *name) {
	while(e) {
		var *v = e->vars;
		while (v) {
			if (strcmp(v->name, name) == 0) {
				return v;
			}
			v = v->next;
		}
		e = e->parent;
	}
	return NULL;
}

int env_assign(env *e, const char *name, value val) {
	var *v = env_lookup(e, name);
	if (!v) {
		return 0;
	}
	v->val = val;
	return 1;
}

void env_define(env *e, const char *name, value val) {
	var *v = malloc(sizeof(var));
	v->name = strdup(name);
	v->val = val;
	v->next = e->vars;
	e->vars = v;
}

/*****************************contructors********************************/

expression *new_literal(int value) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_LITERAL;
	expr->data.value = value;
	return expr;
}

expression *new_binary(operator_type op, expression *left, expression *right) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_BINARY;
	expr->data.binary.operation = op;
	expr->data.binary.left = left;
	expr->data.binary.right = right;
	return expr;
}

expression *new_unary(operator_type op, expression *operand) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_UNARY;
	expr->data.unary.operation = op;
	expr->data.unary.operand = operand;
	return expr;
}

expression *new_variable(char *name) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_VARIABLE;
	expr->data.var_name = name;
	return expr;
}

expression *new_assign(char *name, expression *value) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_ASSIGN;
	expr->data.assign.var_name = name;
	expr->data.assign.value = value;
	return expr;
}

expression *new_print(expression *value) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_PRINT;
	expr->data.print.value = value;
	return expr;
}

expression *new_if(expression *cond, expression *then_branch, expression* else_branch) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_IF;
	expr->data.if_expr.condition = cond;
	expr->data.if_expr.then_branch = then_branch;
	expr->data.if_expr.else_branch = else_branch;
	return expr;
}

expression *new_sequence(expression *left, expression *right) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_SEQUENCE;
	expr->data.sequence.left = left;
	expr->data.sequence.right = right;
	return expr;
}

expression *new_block(expression *body) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_BLOCK;
	expr->data.block.body = body;
	return expr;
}

expression *new_let(char *name, expression *value) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_LET;
	expr->data.let.var_name = name;
	expr->data.let.value = value;
	return expr;
}

expression *new_while(expression *cond, expression *body) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_WHILE;
	expr->data.while_expr.condition = cond;
	expr->data.while_expr.body = body;
	return expr;
}

expression *new_function(char *name, char **params, int param_count, expression *body) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_FUNCTION;
	expr->data.function_expr.name = name;
	expr->data.function_expr.params = params;
	expr->data.function_expr.param_count = param_count;
	expr->data.function_expr.body = body;
	return expr;
}

expression *new_call(expression *callee, expression **args, int arg_count) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_CALL;
	expr->data.call_expr.callee = callee;
	expr->data.call_expr.args = args;
	expr->data.call_expr.arg_count = arg_count;
	return expr;
}

expression *new_return(expression *value) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_RETURN;
	expr->data.return_expr.value = value;
	return expr;
}

/*****************************identifiers*******************************/

int is_identifier_start(char c) {
	return isalpha(c) || c == '_';
}

int is_identifier_char(char c) {
	return isalnum(c) || c == '_';
}


/*****************************evaluation********************************/

value evaluate_expr(expression *expr, env *e) {
	
	switch (expr->type) {

		case EXPR_LITERAL: {
			value v;
			v.type = VAL_INT;
			v.int_val = expr->data.value;
			return v;
		}

		case EXPR_VARIABLE: {
			var *v = env_lookup(e, expr->data.var_name);
			if (!v) {
				fprintf(stderr, "undefined variable: %s\n", expr->data.var_name);
				exit(1);
			}
			return v->val;
		}

		case EXPR_ASSIGN: {
			value val = evaluate_expr(expr->data.assign.value, e);
			
			if (!env_assign(e, expr->data.assign.var_name, val)) {
				fprintf(stderr, "undefined variable: %s\n", expr->data.assign.var_name);
				exit(1);
			}

			return val;
		}

		case EXPR_UNARY: {
			value v = evaluate_expr(expr->data.unary.operand, e);

			if (v.type != VAL_INT) {
				fprintf(stderr, "type error in unary op\n");
				exit(1);
			}

			value result;
			result.type = VAL_INT;

			switch (expr->data.unary.operation) {
				case OP_NEG:
					result.int_val = -v.int_val;
					return result;
				default:
					fprintf(stderr, "unknown unary operator\n");
					exit(1);
			}
		}
		
		case EXPR_IF: {
			value cond = evaluate_expr(expr->data.if_expr.condition, e);

			if (cond.type != VAL_INT) {
				fprintf(stderr, "if condition must be int\n");
				exit(1);
			}

			if (cond.int_val) {
				return evaluate_expr(expr->data.if_expr.then_branch, e);
			}

			if (expr->data.if_expr.else_branch) {
				return evaluate_expr(expr->data.if_expr.else_branch, e);
			}
			
			value result;
			result.type = VAL_INT;
			result.int_val = 0;
			return result;
		}

		case EXPR_BINARY: {
			value l = evaluate_expr(expr->data.binary.left, e);
			value r = evaluate_expr(expr->data.binary.right, e);

			if (l.type != VAL_INT || r.type != VAL_INT) {
				fprintf(stderr, "type error in binary op\n");
				exit(1);
			}

			value result;
			result.type = VAL_INT;

			switch(expr->data.binary.operation) {
				case OP_ADD:
						result.int_val = l.int_val + r.int_val;
						break;
				case OP_SUB:
						result.int_val = l.int_val - r.int_val;
						break;
				case OP_MUL:
						result.int_val = l.int_val * r.int_val;
						break;
				case OP_DIV:
						result.int_val = l.int_val / r.int_val;
						break;
				case OP_MOD:
						result.int_val = l.int_val % r.int_val;
						break;
				case OP_PWR:
						result.int_val = pwr(l.int_val, r.int_val);
						break;

				case OP_EQ:
					result.int_val = (l.int_val == r.int_val);
					break;
				case OP_NEQ:
					result.int_val = (l.int_val != r.int_val);
					break;
				case OP_LT:
					result.int_val = (l.int_val < r.int_val);
					break;
				case OP_GT:
					result.int_val = (l.int_val > r.int_val);
					break;
				case OP_LE:
					result.int_val = (l.int_val <= r.int_val);
					break;
				case OP_GE:
					result.int_val = (l.int_val >= r.int_val);
					break;

				case OP_AND: {
						result.int_val = (l.int_val && r.int_val);
						break;
					}
				case OP_OR: {
						result.int_val = (l.int_val || r.int_val);
						break;
					}

				default:
					fprintf(stderr, "Unknown binary operator\n");
					exit(1);
			}

			return result;
		}
		
		case EXPR_PRINT: {
			value v = evaluate_expr(expr->data.print.value, e);

			if (v.type != VAL_INT) {
				fprintf(stderr, "print expects int\n");
				exit(1);
			}

			printf("%d\n", v.int_val);
			return v;
		}
		
		case EXPR_SEQUENCE: {
			evaluate_expr(expr->data.sequence.left, e);
			if (return_flag) {
				return return_value;
			}
			return evaluate_expr(expr->data.sequence.right, e);
		}

		case EXPR_BLOCK: {
			env *child = new_env(e);
			value result = evaluate_expr(expr->data.block.body, child);
			free_env(child);
			return result;
		}
		
		case EXPR_LET: {
			value val = evaluate_expr(expr->data.let.value, e);
			env_define(e, expr->data.let.var_name, val);
			return val;
		}
		
		case EXPR_WHILE: {
			value result;
			result.type = VAL_INT;
			result.int_val = 0;

			while (1) {
				value cond = evaluate_expr(expr->data.while_expr.condition, e);
				if (cond.type != VAL_INT) {
					fprintf(stderr, "while condition must be int\n");
					exit(1);
				}
				if (!cond.int_val) {
					break;
				}
				result = evaluate_expr(expr->data.while_expr.body, e);
			}

			return result;
		}

		case EXPR_FUNCTION: {
			function *fn = malloc(sizeof(function));
			fn->params = expr->data.function_expr.params;
			fn->param_count = expr->data.function_expr.param_count;
			fn->body = expr->data.function_expr.body;
			fn->closure = e;

			value v;
			v.type = VAL_FUNCTION;
			v.func_val = fn;

			env_define(e, expr->data.function_expr.name, v);

			return v;
		}

		case EXPR_RETURN: {
			return_value = evaluate_expr(expr->data.return_expr.value, e);
			return_flag = 1;
			return return_value;
		}

		case EXPR_CALL: {
			value callee = evaluate_expr(expr->data.call_expr.callee, e);

			if (callee.type != VAL_FUNCTION) {
				fprintf(stderr, "attempt to call non-function\n");
				exit(1);
			}

			function *fn = callee.func_val;
			
			value result;

			if (fn->c_func) {
				value *args = malloc(sizeof(value) * expr->data.call_expr.arg_count);
				for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
					args[i] = evaluate_expr(expr->data.call_expr.args[i], e);
				}

				result = fn->c_func(expr->data.call_expr.arg_count, args);
				free(args);
				return result;
			}

			if (fn->param_count != expr->data.call_expr.arg_count) {
				fprintf(stderr, "argument count mismatch\n");
				exit(1);
			}

			env *call_env = new_env(fn->closure);

			for (int i = 0; i < fn->param_count; i++) {
				value arg = evaluate_expr(expr->data.call_expr.args[i], e);
				env_define(call_env, fn->params[i], arg);
			}

			return_flag = 0;
			result = evaluate_expr(fn->body, call_env);

			if (return_flag) {
				result = return_value;
			}

			return_flag = 0;
			free_env(call_env);

			return result;
		}

		default:
			fprintf(stderr, "Unknown expression type\n");
			exit(1);
	}

}

/******************************free*************************************/

void free_expr(expression *expr) {
	if (!expr) {
		return;
	}

	switch (expr->type) {
		
		case EXPR_LITERAL:
			break;

		case EXPR_VARIABLE:
			free(expr->data.var_name);
			break;

		case EXPR_ASSIGN:
			free(expr->data.assign.var_name);
			free_expr(expr->data.assign.value);
			break;

		case EXPR_UNARY:
			free_expr(expr->data.unary.operand);
			break;

		case EXPR_BINARY:
			free_expr(expr->data.binary.left);
			free_expr(expr->data.binary.right);
			break;

		case EXPR_PRINT:
			free_expr(expr->data.print.value);
			break;

		case EXPR_SEQUENCE:
			free_expr(expr->data.sequence.left);
			free_expr(expr->data.sequence.right);
			break;

		case EXPR_BLOCK:
			free_expr(expr->data.block.body);
			break;

		case EXPR_IF:
			free_expr(expr->data.if_expr.condition);
			free_expr(expr->data.if_expr.then_branch);
			if (expr->data.if_expr.else_branch) {
				free_expr(expr->data.if_expr.else_branch);
			}
			break;

		case EXPR_LET:
			free(expr->data.let.var_name);
			free_expr(expr->data.let.value);
			break;

		case EXPR_WHILE:
			free_expr(expr->data.while_expr.condition);
			free_expr(expr->data.while_expr.body);
			break;

		case EXPR_FUNCTION:
			free(expr->data.function_expr.name);
			for (int i = 0; i < expr->data.function_expr.param_count; i++) {
				free(expr->data.function_expr.params[i]);
			}
			free(expr->data.function_expr.params);
			free_expr(expr->data.function_expr.body);
			break;

		case EXPR_CALL:
			free_expr(expr->data.call_expr.callee);
			for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
				free_expr(expr->data.call_expr.args[i]);
			}
			free(expr->data.call_expr.args);
			break;

		case EXPR_RETURN:
			free_expr(expr->data.return_expr.value);
			break;

		default:
			break;
	}
	free(expr);
}

void free_env(env *e) {
	var *v = e->vars;
	while (v) {
		var *next = v->next;
		free(v->name);

		if (v->val.type == VAL_FUNCTION) {
			function *fn = v->val.func_val;

			free(fn);
		}

		free(v);
		v = next;
	}
	free(e);
}

/*******************************parsing*********************************/

expression *parse_number(parser *p) {
	skip_ws(p);

	int val = 0;

	while (isdigit(peek(p))) {
		val = val * 10 + (advance(p) - '0');
	}
	
	return new_literal(val);
}

expression *parse_factor(parser *p) {
	skip_ws(p);

	if (peek(p) == '(') {
		advance(p);
		expression *expr = parse_expression(p);
		skip_ws(p);
		if (peek(p) == ')') {
			advance(p);
		} else {
			fprintf(stderr, "expected ')'\n");
			exit(1);
		}
		return expr;
	}

	char *name = parse_identifier(p);
	if (name) {
		skip_ws(p);
		if (peek(p) == '(') {
			advance(p);
			skip_ws(p);

			expression **args = NULL;
			int arg_count = 0;

			if (peek(p) != ')') {
				while (1) {
					expression *arg = parse_expression(p);
					args = realloc(args, sizeof(expression*) * (arg_count + 1));
					args[arg_count++] = arg;
					skip_ws(p);
					if (peek(p) == ',') {
						advance(p);
						skip_ws(p);
					} else {
						break;
					}
				}
			}

			if (peek(p) != ')') {
				fprintf(stderr, "expected ')'\n");
				exit(1);
			}

			advance(p);

			expression *callee = new_variable(name);
			return new_call(callee, args, arg_count);
		}
		return new_variable(name);
	}

	return parse_number(p);
}

expression *parse_term(parser *p) {
	expression *left = parse_power(p);

	while (1) {
		skip_ws(p);
		char c = peek(p);

		if (c == '*' || c == '/' || c == '%') {
			advance(p);
	
			operator_type op;
			
			if (c == '*') {
				op = OP_MUL;
			} else if (c == '/') {
				op = OP_DIV;
			} else {
				op = OP_MOD;
			}

			expression *right = parse_power(p);
			left = new_binary(op, left, right);
		} else {
			break;
		}
	}

	return left;
}

expression *parse_additive(parser *p) {
	expression *left = parse_term(p);
	 
	while (1) {
		skip_ws(p);
		char c = peek(p);
		
		if (c == '+' || c == '-') {
			advance(p);

			operator_type op = (c == '+') ? OP_ADD : OP_SUB;

			expression *right = parse_term(p);
			left = new_binary(op, left, right);
		} else {
			break;
		}
	}

	return left;
}

expression *parse_comparison(parser *p) {
	expression *left = parse_additive(p);

	while (1) {
		skip_ws(p);

		operator_type op;

		if (peek(p) == '=' && p->input[p->pos + 1] == '=') {
			p->pos += 2;
			op = OP_EQ;
		} else if (peek(p) == '!' && p->input [p->pos + 1] == '=') {
			p->pos += 2;
			op = OP_NEQ;
		} else if (peek(p) == '<' && p->input [p->pos + 1] == '=') {
			p->pos += 2;
			op = OP_LE;
		} else if (peek(p) == '>' && p->input [p->pos + 1] == '=') {
			p->pos += 2;
			op = OP_GE;
		} else if (peek(p) == '<') {
			advance(p);
			op = OP_LT;
		} else if (peek(p) == '>') {
			advance(p);
			op = OP_GT;
		} else {
			break;
		}

		expression *right = parse_additive(p);
		left = new_binary(op, left, right);
	}

	return left;
}

expression *parse_expression(parser *p) {
	return parse_logical_or(p);
}

expression *parse_unary(parser *p) {
	skip_ws(p);

	if (peek(p) == '-') {
		advance(p);
		expression *operand = parse_unary(p);
		return new_unary(OP_NEG, operand);
	}

	return parse_factor(p);
}

expression *parse_power(parser *p) {
	expression *left = parse_unary(p);

	skip_ws(p);

	if (peek(p) == '^') {
		advance(p);

		expression *right = parse_power(p);
		return new_binary(OP_PWR, left, right);
	}

	return left;
}

char *parse_identifier(parser *p) {
	skip_ws(p);

	if (!is_identifier_start(peek(p))) {
		return NULL;
	}

	int start = p->pos;

	while (is_identifier_char(peek(p))) {
		advance(p);
	}

	int len = p->pos - start;

	char *name = malloc(len + 1);
	strncpy(name, p->input + start, len);
	name[len] = '\0';

	return name;
}

expression *parse_statement(parser *p) {
	expression *left = parse_if(p);

	skip_ws(p);

	if (peek(p) == ';') {
		advance(p);
		expression *right = parse_statement(p);
		return new_sequence(left, right);
	}

	return left;
}

expression *parse_if(parser *p) {
	skip_ws(p);

	if (strncmp(p->input + p->pos, "let", 3) == 0 && !isalnum(p->input[p->pos + 3])) {
		p->pos += 3;
		skip_ws(p);

		char *name = parse_identifier(p);
		if (!name) {
			fprintf(stderr, "expected identifer after let\n");
			exit(1);
		}

		skip_ws(p);

		if (peek(p) != '=') {
			fprintf(stderr, "expected '=' after identifier\n");
			exit(1);
		}

		advance(p);

		expression *value = parse_expression(p);

		return new_let(name, value);
	}

	if (strncmp(p->input + p->pos, "fun", 3) == 0 && !isalnum(p->input[p->pos + 3])) {
		p->pos += 3;
		skip_ws(p);

		char *name = parse_identifier(p);
		if (!name) {
			fprintf(stderr, "expected function name\n");
			exit(1);
		}

		skip_ws(p);

		if (peek(p) != '(') {
			fprintf(stderr, "expected '('\n");
			exit(1);
		}

		advance(p);
		skip_ws(p);

		char **params = NULL;
		int param_count = 0;

		if (peek(p) != ')') {
			while (1) {
				char *param = parse_identifier(p);
				if (!param) {
					fprintf(stderr, "expected parameter name\n");
					exit(1);
				}

				params = realloc(params, sizeof(char*) * (param_count + 1));
				params[param_count++] = param;
				skip_ws(p);

				if (peek(p) == ',') {
					advance(p);
					skip_ws(p);
				} else {
					break;
				}
			}
		}

		if (peek(p) != ')') {
			fprintf(stderr, "expected ')'\n");
			exit(1);
		}

		advance(p);
		skip_ws(p);

		if (peek(p) != '{') {
			fprintf(stderr, "expected '{'\n");
			exit(1);
		}

		expression *body = parse_if(p);
		return new_function(name, params, param_count, body);
	}

	if (strncmp(p->input + p->pos, "return", 6) == 0 && !isalnum(p->input[p->pos+6])) {
		p->pos += 6;
		skip_ws(p);

		expression *value = parse_expression(p);
		return new_return(value);
	}

	if (strncmp(p->input + p->pos, "if", 2) == 0 && !isalnum(p->input[p->pos + 2])) {
		p->pos += 2;
		skip_ws(p);

		if (peek(p) != '(') {
			fprintf(stderr, "expected '('\n");
			exit(1);
		}

		advance(p);
		expression *cond = parse_expression(p);

		skip_ws(p);

		if (peek(p) != ')') {
			fprintf(stderr, "expected ')'\n");
			exit(1);
		}

		advance(p);
		expression *then_branch = parse_if(p);
		
		skip_ws(p);
		expression *else_branch = NULL;

		if (strncmp(p->input+p->pos, "else", 4) == 0 && !isalnum(p->input[p->pos + 4])) {
			p->pos += 4;
			else_branch = parse_if(p);
		}

		return new_if(cond, then_branch, else_branch);
	}

	if (strncmp(p->input + p->pos, "while", 5) == 0 && !isalnum(p->input[p->pos+5])) {
		p->pos += 5;
		skip_ws(p);
		
		if (peek(p) != '(') {
			fprintf(stderr, "expected '(' after while\n");
			exit(1);
		}

		advance(p);
		expression *cond = parse_expression(p);
		skip_ws(p);

		if (peek(p) != ')') {
			fprintf(stderr, "expected ')' after while\n");
			exit(1);
		}

		advance(p);
		expression *body = parse_if(p);
		return new_while(cond, body);
	}

	if (peek(p) == '{') {
		advance(p);
		
		expression *body = NULL;
		
		skip_ws(p);
		
		while (peek(p) != '}' && !is_at_end(p)) {
			expression *stmt = parse_statement(p);

			if (!body) {
				body = stmt;
			} else {
				body = new_sequence(body, stmt);
			}

			skip_ws(p);
		}

		if (peek(p) != '}') {
			fprintf(stderr, "expected '}'\n");
			exit(1);
		}

		advance(p);

		return new_block(body);
	}

	return parse_assignment(p);
}

expression *parse_assignment(parser *p) {
	expression *left = parse_logical_or(p);

	skip_ws(p);

	if (peek(p) == '=') {
		advance(p);

		expression *right = parse_assignment(p);

		if (left->type != EXPR_VARIABLE) {
			fprintf(stderr, "invalid assignment target\n");
			exit(1);
		}

		return new_assign(left->data.var_name, right);
	}

	return left;
}

expression *parse_logical_and(parser *p) {
	expression *left = parse_comparison(p);

	while (1) {
		skip_ws(p);

		if (peek(p) == '&' && p->input[p->pos + 1] == '&') {
			p->pos += 2;
			expression *right = parse_comparison(p);
			left = new_binary(OP_AND, left, right);
		} else {
			break;
		}
	}

	return left;
}

expression *parse_logical_or(parser *p) {
	expression *left = parse_logical_and(p);

	while(1) {
		skip_ws(p);

		if (peek(p) == '|' && p->input[p->pos + 1] == '|') {
			p->pos += 2;
			expression *right = parse_logical_and(p);
			left = new_binary(OP_OR, left, right);
		} else {
			break;
		}	
	}

	return left;
}

expression *parse_program(parser *p) {
	expression *body = NULL;

	while (!is_at_end(p)) {
		expression *stmt = parse_statement(p);

		if (!body) {
			body = stmt;
		} else {
			body = new_sequence(body, stmt);
		}

		skip_ws(p);
	}

	return body;
}

/***********************************************************************/

static void print_cont_prompt(int indent_level) {
	printf(COLOR_CONT);

	for (int i = 0; i < indent_level; i++) {
		printf("    "); //4 spaces
	}

	printf("... " COLOR_RESET);
}

/***********************************************************************/

void run_repl() {
	printf(COLOR_HEAD "ZCALC INT REPL\n" COLOR_RESET);
	printf("( + - * / ^ %% < <= >= > == != && || )\n");
	env *global = new_env(NULL);

	function *print_fn = malloc(sizeof(function));
	print_fn->param_count = 1;
	print_fn->params = malloc(sizeof(char*));
	print_fn->params[0] = strdup("x");
	print_fn->body = NULL;
	print_fn->closure = NULL;
	print_fn->c_func = builtin_print;

	value v;
	v.type = VAL_FUNCTION;
	v.func_val = print_fn;

	env_define(global, "print", v);

	while (1) {
		char buffer[4096] = {0};
		int brace_depth = 0;
		int paren_depth = 0;

		printf(COLOR_PROMPT "zc> " COLOR_RESET);

		while (1) {
			char line[256];

			if (!fgets(line, sizeof(line), stdin)) {
				printf("\n");
				free_env(global);
				return;
			}

			strcat(buffer, line);

			for (int i = 0; line[i]; i++) {
				if (line[i] == '{') {
					brace_depth++;
				}
				if (line[i] == '}') {
					brace_depth--;
				}
				if (line[i] == '(') {
					paren_depth++;
				}
				if (line[i] == ')') {
					paren_depth--;
				}
			}

			if (brace_depth <= 0 && paren_depth <= 0) {
				break;
			}

			print_cont_prompt(brace_depth);
		}

		parser p = { buffer, 0 };
		expression *expr = parse_program(&p);
		if (!expr) {
			fprintf(stderr, "Parse error...\n");
			continue;
		}
		return_flag = 0;
		evaluate_expr(expr, global);
		//free_expr(expr); MEMORY LEAK !!!!! NEED TO IMPLEMENT COPYING
	}

	free_env(global);
}

char *read_file(const char *filename) {
	FILE *f = fopen(filename, "rb");
	if (!f) {
		perror("Could not open file");
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	char *buffer = malloc(size + 1);
	if (!buffer) {
		perror("malloc failed");
		exit(1);
	}

	fread(buffer, 1, size, f);
	buffer[size] = '\0';

	fclose(f);
	return buffer;
}

void run_file(const char *filename) {
	char *source = read_file(filename);

	env *global = new_env(NULL);

	function *print_fn = malloc(sizeof(function));
	print_fn->param_count = 1;
	print_fn->params = malloc(sizeof(char*));
	print_fn->params[0] = strdup("x");
	print_fn->body = NULL;
	print_fn->closure = NULL;
	print_fn->c_func = builtin_print;

	value v;
	v.type = VAL_FUNCTION;
	v.func_val = print_fn;

	env_define(global, "print", v);

	parser p = { source, 0 };	
	expression *expr = parse_program(&p);

	evaluate_expr(expr, global);
	
	free_expr(expr);
	free(source);
	free_env(global);
}

/*********************************main**********************************/

int main(int argc, char **argv) {
	if (argc == 1) {
		run_repl();
	} else if (argc == 2) {
		run_file(argv[1]);
	} else {
		fprintf(stderr, "usage: %s [file.zc]\n", argv[0]);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
