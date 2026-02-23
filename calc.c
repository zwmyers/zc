#include "calc.h"

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

expression *new_if(expression *cond, expression *then_branch) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_IF;
	expr->data.if_expr.condition = cond;
	expr->data.if_expr.then_branch = then_branch;
	return expr;
}

expression *new_sequence(expression *left, expression *right) {
	expression *expr = malloc(sizeof(expression));
	expr->type = EXPR_SEQUENCE;
	expr->data.sequence.left = left;
	expr->data.sequence.right = right;
	return expr;
}

/*****************************identifiers*******************************/

int is_identifier_start(char c) {
	return isalpha(c) || c == '_';
}

int is_identifier_char(char c) {
	return isalnum(c) || c == '_';
}

/******************************variables********************************/

variable vars[MAX_VARS];
int var_count = 0;

int get_var(const char *name) {
	for (int i = 0; i < var_count; i++) {
		if (strcmp(vars[i].name, name) == 0) {
			return vars[i].value;
		}
	}
	fprintf(stderr, "undefined variable: %s\n", name);
	return -1;
}

void set_var(const char *name, int value) {
	for (int i = 0; i < var_count; i++) {
		if (strcmp(vars[i].name, name) == 0) {
			vars[i].value = value;
			return;
		}
	}

	strcpy(vars[var_count].name, name);
	vars[var_count].value = value;
	var_count++;
}

/*****************************evaluation********************************/

int evaluate_expr(expression *expr) {
	
	switch (expr->type) {

		case EXPR_LITERAL:
			return expr->data.value;

		case EXPR_VARIABLE:
			return get_var(expr->data.var_name);

		case EXPR_ASSIGN: {
			int val = evaluate_expr(expr->data.assign.value);
			set_var(expr->data.assign.var_name, val);
			return val;
		}

		case EXPR_UNARY: {
			int val = evaluate_expr(expr->data.unary.operand);

			switch (expr->data.unary.operation) {
				case OP_NEG:
					return -val;
				default:
					fprintf(stderr, "unknown unary operator\n");
					exit(1);
			}
		}
		
		case EXPR_IF: {
			int cond = evaluate_expr(expr->data.if_expr.condition);

			if (cond) {
				return evaluate_expr(expr->data.if_expr.then_branch);
			}

			return 0;
		}

		case EXPR_BINARY: {
			operator_type op = expr->data.binary.operation;
			expression *left_expr = expr->data.binary.left;
			expression *right_expr = expr->data.binary.right;

			switch(op) {
				case OP_ADD:
						return evaluate_expr(left_expr) + evaluate_expr(right_expr);
				case OP_SUB:
						return evaluate_expr(left_expr) - evaluate_expr(right_expr);
				case OP_MUL:
						return evaluate_expr(left_expr) * evaluate_expr(right_expr);
				case OP_DIV:
						return evaluate_expr(left_expr) / evaluate_expr(right_expr);
				case OP_MOD:
						return evaluate_expr(left_expr) % evaluate_expr(right_expr);
				case OP_PWR:
						return pwr(evaluate_expr(left_expr), evaluate_expr(right_expr));

				case OP_EQ:
					return evaluate_expr(left_expr) == evaluate_expr(right_expr);
				case OP_NEQ:
					return evaluate_expr(left_expr) != evaluate_expr(right_expr);
				case OP_LT:
					return evaluate_expr(left_expr) < evaluate_expr(right_expr);
				case OP_GT:
					return evaluate_expr(left_expr) > evaluate_expr(right_expr);
				case OP_LE:
					return evaluate_expr(left_expr) <= evaluate_expr(right_expr);
				case OP_GE:
					return evaluate_expr(left_expr) >= evaluate_expr(right_expr);

				case OP_AND: {
						int left_val = evaluate_expr(left_expr);
						return (!left_val) ? 0 : (evaluate_expr(right_expr) != 0);
					}
				case OP_OR: {
						int left_val = evaluate_expr(left_expr);
						return left_val ? 1 : (evaluate_expr(right_expr) != 0);
					}

				default:
					fprintf(stderr, "Unknown binary operator\n");
					exit(1);
			}
		}
		
		case EXPR_PRINT: {
			int val = evaluate_expr(expr->data.print.value);
			printf("%d\n", val);
			return val;
		}
		
		case EXPR_SEQUENCE: {
			evaluate_expr(expr->data.sequence.left);
			return evaluate_expr(expr->data.sequence.right);
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
	
	if (expr->type == EXPR_ASSIGN) {
		free(expr->data.assign.var_name);
		free_expr(expr->data.assign.value);
	}

	if (expr->type == EXPR_BINARY) {
		free_expr(expr->data.binary.left);
		free_expr(expr->data.binary.right);
	}

	free(expr);
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
			expression *arg = parse_expression(p);
			skip_ws(p);
			if (peek(p) != ')') {
				fprintf(stderr, "expected ')'\n");
				exit(1);
			}
			advance(p);
			if (strcmp(name, "print") == 0) {
				free(name);
				return new_print(arg);
			}
			fprintf(stderr, "unknown function: %s\n", name);
			exit(1);
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

		return new_if(cond, then_branch);
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

/***********************************************************************/

void run_repl() {
	printf("ZCALC INT REPL\n");
	printf("( + - * / ^ %% < <= >= > == != && || )\n");

	while (1) {
		char line[256];

		printf(">>");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		line[strcspn(line, "\n")] = '\0';

		parser p = { line, 0 };
		expression *expr = parse_statement(&p);

		int val = evaluate_expr(expr);
		if (expr->type != EXPR_PRINT) {
			//printf("%d\n", val);
		}

		free_expr(expr);
	}
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

	parser p = { source, 0 };
	
	expression *expr = parse_statement(&p);

	evaluate_expr(expr);
	
	free_expr(expr);
	free(source);
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
