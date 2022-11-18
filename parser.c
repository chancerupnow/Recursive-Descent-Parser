#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define ARRAY_SIZE 500

typedef enum token_type {
	identifier = 1, number, keyword_const, keyword_var, keyword_procedure,
	keyword_call, keyword_begin, keyword_end, keyword_if, keyword_then,
	keyword_else, keyword_while, keyword_do, keyword_read, keyword_write,
	keyword_def, period, assignment_symbol, minus, semicolon,
	left_curly_brace, right_curly_brace, equal_to, not_equal_to, less_than,
	less_than_or_equal_to, greater_than, greater_than_or_equal_to, plus, times,
	division, left_parenthesis, right_parenthesis
} token_type;

typedef enum opcode_name {
	LIT = 1, OPR = 2, LOD = 3, STO = 4, CAL = 5, INC = 6, JMP = 7, JPC = 8, 
	SYS = 9, WRT = 1, RED = 2, HLT = 3, 
	RTN = 0, ADD = 1, SUB = 2, MUL = 3, DIV = 4, EQL = 5, NEQ = 6,
	LSS = 7, LEQ = 8, GTR = 9, GEQ = 10
} opcode_name;

typedef struct lexeme {
	token_type type;
	char identifier_name[12];
	int number_value;
	int error_type;
} lexeme;

typedef struct instruction {
	int op;
	int l;
	int m;
} instruction;

typedef struct symbol {
	int kind;
	char name[12];
	int value;
	int level;
	int address;
	int mark;
} symbol;

lexeme *tokens;
int token_index = 0;
symbol *table;
int table_index = 0;
instruction *code;
int code_index = 0;

int error = 0;
int level;

// given functions
void emit(int op, int l, int m);
void add_symbol(int kind, char name[], int value, int level, int address);
void mark();
int multiple_declaration_check(char name[]);
int find_symbol(char name[], int kind);

// given print functions
void print_parser_error(int error_code, int case_code);
void print_assembly_code();
void print_symbol_table();

// MY CODE CALLS
void program();
void block();
int declarations();
void constants();
void variables(int numVars);
void procedures();
void statement();
void factor();

int main(int argc, char *argv[])
{
	// variable setup
	int i;
	tokens = calloc(ARRAY_SIZE, sizeof(lexeme));
	table = calloc(ARRAY_SIZE, sizeof(symbol));
	code = calloc(ARRAY_SIZE, sizeof(instruction));
	FILE *ifp;
	int buffer;
	
	// read in input
	if (argc < 2)
	{
		printf("Error : please include the file name\n");
		return 0;
	}
	
	ifp = fopen(argv[1], "r");
	while(fscanf(ifp, "%d", &buffer) != EOF)
	{
		tokens[token_index].type = buffer;
		if (buffer == identifier)
			fscanf(ifp, "%s", tokens[token_index].identifier_name);
		else if (buffer == number)
			fscanf(ifp, "%d", &(tokens[token_index].number_value));
		token_index++;
	}
	fclose(ifp);
	token_index = 0;
	
	/* print out tokens to visualize initial input
	for(int k = 0; k < ARRAY_SIZE; k++) {
		if(tokens[k].type != 0) {
			printf("%d %s %d %d\n", tokens[k].type, tokens[k].identifier_name, 
			tokens[k].number_value, tokens[k].error_type);
		}
	} */

	// call program
	program();
	
	free(tokens);
	free(table);
	free(code);
	return 0;
}

// program function
void program() {

	//printf("start program\n");

	// add symbol to end of table
	add_symbol(3, "main", 0, 0, 0);

	// set level to -1
	level = -1;

	// emit jmp, M = 0, L = 0
	emit(JMP, 0, 0);

	//printf("program before block\n");

	// call block()
	block();

	//printf("program after block\n");

	// if error, stop execution
	if(error == -1) {
		return;
	}

	//printf("%d\n", token_index);

	// if current token != period
	if(tokens[token_index].type != period) {

		// error 1, return
		print_parser_error(1, 0);

		// error = -1;
		error = -1;

		// stop execution
		return;
	}
	
	// for each CAL instruction in code
	for(int j = 0; j < code_index; j++) {

		// if is cal
		if(code[j].op == CAL) {

			// set M val of the instruction to the address of the procedure
			code[j].m = table[code[j].m].address;

		}

	}

	// since we know main's address, fix initial jump
	// main is first entry in symbol table
	// edit code array at index 0
	code[0].m = table[0].address;

	// emit HLT, L = 0
	emit(SYS, 0, HLT);

	//printf("print point\n");

	// print assembly code and table
	print_assembly_code();
	print_symbol_table();

	// END OF PROGRAM()
}

// block function
void block() {
	// the very last symbol added to the symbol table was the current procedure, 
	// whether this was main or a subprocedure, we need to save where the procedure
	//  is in the symbol table before we add more symbols, so we can use it to set 
	// the address before we emit code in statement

	//printf("block before declarations\n");

	// increment level
	level++;

	// inc_m_value to declarations call
	int inc_m_value = declarations();

	// if error, return
	if(inc_m_value == -1) {
		error = -1;
		return;
	}

	//printf("block before procedures\n");

	procedures();

	// once we emit INC, we'll be emitting code so this is where the procedure starts, 
	// multiply by 3 bc PAS format
	table[table_index].address = code_index * 3;

	// emit() INC (m = inc_m_value)
	emit(INC, 0, inc_m_value);

	//printf("block before state\n");

	// call statement
	statement();

	//printf("block after state\n");

	// if error, return
	if(error == -1) {
		return;
	}

	// call mark
	mark();

	// decrement level
	level--;

	//printf("end block\n");

	// END OF BLOCK()
}

// declarations function
int declarations() {

	//printf("start declarations\n");

	// set num of variables declared to 0
	int number_of_variables_declared = 0;

	// while current token == keyword_const || keyword_var
	while(tokens[token_index].type == keyword_const || keyword_var ){

		// if current token == keyword_const
		if(tokens[token_index].type == keyword_const){

			//printf("declarations before constants\n");

			// call constants
			constants();

			// if error, return
			if(error == -1) {
				return -1;
			}

		}
		
		// else
		else if(tokens[token_index].type == keyword_var){

			//printf("declarations before var\n");

			// call variables
			variables(number_of_variables_declared);

			// if error, return
			if(error == -1) {
				return -1;
			}

			// increment number_of_variables_declared
			number_of_variables_declared++;

		}

		else{
			break;
		}

	}

	//printf("end of declarations\n");
	
	return number_of_variables_declared + 3; //(bc of ar size)

	// END OF DECLARATIONS()
}

// constants function
void constants() {

	//printf("start of const\n");

	// set optional minus flag to false
	// using <stdbool.h> to define booleans
	bool minus_flag = false;

	// move to next token
	token_index++;

	// if current token != identifier
	if(tokens[token_index].type != identifier) {

		// error 2-1, return
		print_parser_error(2, 1);

		// set error flag to -1
		error = -1;

		// return
		return;

	}
	
	// if (multiple_declaration_check(identifier_name) != -1)
	if(multiple_declaration_check(tokens[token_index].identifier_name) != -1) {

		// this means that the identifier name has already been used by another 
		// symbol in this procedure

		// error 3, return
		print_parser_error(3, 0);

		// set error flag to -1
		error = -1;

		// return
		return;

	}
	
	// save the identifier_name for the symbol name
	strcpy(table[table_index].name, tokens[token_index].identifier_name);

	// move to next token
	token_index++;

	// if current token != assignment_symbol
	if(tokens[token_index].type != assignment_symbol){

		// error 4-1, return
		print_parser_error(4, 1);

		// set error flag to -1
		error = -1;

		// return
		return;

	}

	// move to next token
	token_index++;
	
	// if current token == minus
	if(tokens[token_index].type == minus){

		// set minus_flag to true
		minus_flag = true;

		// move to next token
		token_index++;

	}

	// if current token != number
	if(tokens[token_index].type != number) {

		// error 5, return
		print_parser_error(5, 0);

		// set error flag to -1
		error = -1;

		// return
		return;

	}

	// save number_value for symbol table
	table[table_index].value = tokens[token_index].number_value;

	// move to next token
	token_index++;

	if(minus_flag == true) {

		// symbol value * -1;
		table[table_index].value *= -1;

	}

	// add_symbol(1, identifier_name, number_value, level, 0);
	add_symbol(1, table[table_index].name, table[table_index].value, level, 0);

	// if current token != semicolon
	if(tokens[token_index].type != semicolon){

		// error 6-1, return
		print_parser_error(6, 1);

		// set error flag to -1
		error = -1;

		// return
		return;

	}

	// move to next token
	token_index++;

	//printf("end of const\n");

	// END OF CONSTANTS()
}

// variables function
void variables(int numVars) {

	//printf("begin var\n");

	//printf("%d\n", tokens[token_index].type);

	// move to next token
	token_index++;

	// if current token != identifier
	if(tokens[token_index].type != identifier){

		// error 2-2, return
		print_parser_error(2, 2);

		// set error flag to -1
		error = -1;

		// return
		return;

	}

	// if multiple_declaration_check(identifier_name) != -1
	if(multiple_declaration_check(tokens[token_index].identifier_name) != -1){

		// this means that the identifier name has already been used 
		// by another symbol in this procedure

		// error 3, return
		print_parser_error(3, 0);

		// set error flag to -1
		error = -1;

		// return
		return;

	}
	
	// save the identifier_name for the symbol name
	strcpy(table[table_index].name, tokens[token_index].identifier_name);

	// move to next token
	token_index++;

	//printf("%d\n", token_index);
	//printf("%d\n", numVars);

	// add_symbol(2, identifier_name, 0, level, numVars + 3)
	add_symbol(2, table[table_index].name, 0, level, numVars + 3);

	// if current token != semicolon
	if(tokens[token_index].type != semicolon){

		// error 6-2, return
		print_parser_error(6, 2);

		// set error flag to -1
		error = -1;

		// return
		return;

	}

	// move to next token
	token_index++;

	//printf("end var\n");

	// END OF VARIABLES()
}

// procedures function
void procedures() {

	//printf("start proc\n");

	// while current token == keyword_procedure
	while(tokens[token_index].type == keyword_procedure){

		// move to next token
		token_index++;

		// if current token != identifier
		if(tokens[token_index].type != identifier){

			// error 2-3, return
			print_parser_error(2, 3);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		// if multiple_declaration_check(identifier_name) != -1
		if(multiple_declaration_check(tokens[token_index].identifier_name) != -1){

			// this means that the identifier name has already 
			// been used by another symbol in this procedure

			// error 3, return
			print_parser_error(3, 0);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		// save the identifier_name for the symbol name
		strcpy(table[table_index].name, tokens[token_index].identifier_name);
		
		// move to next token
		token_index++;

		// add_symbol(3, identifier_name, 0, level, 0)
		add_symbol(3, table[table_index].name, 0, level, 0);

		// if current token != left_curly_brace
		if(tokens[token_index].type != left_curly_brace) {

			// error 14, return
			print_parser_error(14, 0);

			// set error flag to -1
			error = -1;

			// return
			return;

		}
		
		// move to next token
		token_index++;

		//printf("proc before block\n");

		// block();
		block();

		// if error, return
		if(error == -1) {
			return;
		}

		// emit() RTN
		emit(OPR, 0, RTN);

		// if current token != right_curly_brace
		if(tokens[token_index].type != right_curly_brace){

			// error 15, return
			print_parser_error(15, 0);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		// move to next token
		token_index++;

	}

	//printf("end proc\n");

	// END OF PROCEDURES()
}

// statement function
void statement() {

	//printf("start state\n");

	//printf("%d\n", token_index);

	// if current token == keyword_def
	if(tokens[token_index].type == keyword_def){

		// move to next token
		token_index++;

		// if current token != identifier
		if(tokens[token_index].type != identifier){

			// error 2-6, return
			print_parser_error(2, 6);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		int symbol_index_in_table = find_symbol(tokens[token_index].identifier_name, 2);
		
		// if symbol_index_in_table == -1 // couldn't find it
		if(symbol_index_in_table == -1) {

			// if find_symbol(identifier_name, 1) == find_symbol(identifier_name, 3);
			if(find_symbol(tokens[token_index].identifier_name, 1) == find_symbol(tokens[token_index].identifier_name, 3)) {

				// this will only be true if there isn’t a constant AND there 
				// isn’t a procedure with the desired name

				// error 8-1, return
				print_parser_error(8, 1);

				// set error flag to -1
				error = -1;

				// return
				return;

			}
			
			// else // there was a constant and/or procedure
			else{

				// error 7, return
				print_parser_error(7, 0);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

		}
		
		// move to next token
		token_index++;

		// if current token != assignment_symbol
		if(tokens[token_index].type != assignment_symbol){

			// error 4-2, return
			print_parser_error(4, 2);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		// move to next token
		token_index++;

		//printf("state before factor\n");

		// factor();
		factor();

		// if error, return
		if(error == -1) {
			return;
		}

		// emit() STO, L = level, m = symbol's address from table
		emit(STO, level - table[symbol_index_in_table].level, table[symbol_index_in_table].address);

	}

	// else if current token == keyword_call
	else if (tokens[token_index].type == keyword_call){

		// move to next token
		token_index++;

		// if current token != identifier
		if(tokens[token_index].type != identifier){

			// error 2-4, return
			print_parser_error(2, 4);

			// set error flag to -1
			error = -1;

			// return
			return;

		}

		// symbol_index_in_table = find_symbol(identifier_name, 3)
		int symbol_index_in_table = find_symbol(tokens[token_index].identifier_name, 3);

		// if symbol_index_in_table == -1 // we couldn't find it
		if(symbol_index_in_table == -1) {

			// if find_symbol(indtifier_name, 1) == find_symbol(identifier_name, 2)
			if(find_symbol(tokens[token_index].identifier_name, 1) == find_symbol(tokens[token_index].identifier_name, 2)){

				// this will only be true if there isn’t a constant AND 
				// there isn’t a variable with the desired name

				// error 8-2, return
				print_parser_error(8, 2);

				// set error flag to -1
				error = -1;

				// return
				return;

			}
			
			// else // there was a constant and/or variable
			else{

				// error 9, return
				print_parser_error(9, 0);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

		}
		
		// move to next token
		token_index++;

		// emit CAl, L = level, m = symbol_index_in_table
		emit(CAL, level, symbol_index_in_table);

		// we do this because our procedure may not have been defined yet, 
		// and this way we can go back later, find it in the table, and get
	 	// the address after they’ve all been defined

	}

	// else if current token == keyword_begin
	else if(tokens[token_index].type == keyword_begin){

		// do 
		do{

			// move to next token
			token_index++;

			// statement();
			statement();

			// if error, return
			if(error == -1) {
				return;
			}

		}
		
		// while current token == semicolon
		while(tokens[token_index].type == semicolon);

		// if current token != keyword_end
		if(tokens[token_index].type != keyword_end){

			// if current token == identifier || keyword_call ||
			// keyword_begin || keyword_read || keyword_def
			if(tokens[token_index].type == identifier || keyword_call || keyword_begin || keyword_read || keyword_def){

				// this means that there was a semicolon missing 
				// between two statements

				// error 6-3, return
				print_parser_error(6, 3);

				// set error flag to -1
				error = -1;

				// return
				return;

			}
			
			// else 
			else {

				// error 10, return
				print_parser_error(10, 0);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

		}
			
		// move to next token
		token_index++;

	}
		
		// else if current token == keyword_read
		else if(tokens[token_index].type == keyword_read){

			// move to next token
			token_index++;

			// if current token != identifier
			if(tokens[token_index].type != identifier){

				// error 2-5, return
				print_parser_error(2, 5);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

			//printf("%s\n", tokens[token_index].identifier_name);
			//printf("%s\n", table[1].name);
			//printf("%d\n", table[1].kind);

			// symbol_index_in_table = find_symbol(identifier_name, 2);
			int symbol_index_in_table = find_symbol(tokens[token_index].identifier_name, 2);
			//printf("%d\n", symbol_index_in_table);


			// if symbol_index_in_table == -1 // we couldn't find it
			if(symbol_index_in_table == -1){

				// if find_symbol(identifier_name, 1) == find_symbol(identifier_name, 3)
				if(find_symbol(tokens[token_index].identifier_name, 1) == find_symbol(tokens[token_index].identifier_name, 3)){

					// this will only be true if there isn’t a constant AND 
					// there isn’t a procedure with the desired name

					// error 8-3, return
					print_parser_error(8, 3);

					// set error flag to -1
					error = -1;

					// return
					return;

				}

				// else // there was a constant or procedure
				else {

					// error 13, return
					print_parser_error(13, 0);

					// set error flag to -1
					error = -1;

					// return
					return;

				}

			}
				
			// move to next token
			token_index++;

			// emit RED
			emit(SYS, 0, RED);

			// emit STO, L = level, M = symbol's address from table
			emit(STO, level - table[symbol_index_in_table].level, table[symbol_index_in_table].address);

		}

	//printf("%d\n", token_index);

	//printf("end of state\n");

	// END OF STATEMENT()
}

// factor function
void factor() {

	//printf("start of factor\n");

	// if current token == identifier
	if(tokens[token_index].type == identifier){

		// constant_index = find_symbol(indentifier_name, 1);
		int constant_index = find_symbol(tokens[token_index].identifier_name, 1);

		// variable_index = find_symbol(indentifier_name, 2);
		int variable_index = find_symbol(tokens[token_index].identifier_name, 2);

		// if (constant_index == variable_index)
		if(constant_index == variable_index) {

			// this will only happen if there wasn’t a constant 
			// AND there wasn’t a variable with the desired name

			// if find_symbol(identifier_name, 3) != -1
			if(find_symbol(tokens[token_index].identifier_name, 3) != -1){

				// there is a valid procedure

				// error 17, return
				print_parser_error(17, 0);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

			// else 
			else{

				//printf("%d\n", token_index);
				//printf("%s\n", tokens[token_index].identifier_name);
				//printf("%s\n", table[5].name);

				// error 8-4, return
				print_parser_error(8, 4);

				// set error flag to -1
				error = -1;

				// return
				return;

			}

		}
			
		// if constant_index == -1
		if(constant_index == -1) {

			// emit LOD, L = level, M = address of variable from table
			emit(LOD, level - table[variable_index].level, table[variable_index].address);

		}
		
		// else if var_index == -1
		else if (variable_index == -1) {

			// emit LIT , M = value of constant from table
			emit(LIT, 0, table[constant_index].value);

		}

		// else if level of constant from table > level of variable from table
		else if(table[constant_index].level > table[variable_index].level){

			// emit LIT, m = value of constant from table
			emit(LIT, 0, table[constant_index].value);

		}

		// else
		else {

			// emit LOD, L = level, M = address of variable from table
			emit(LOD, level - table[variable_index].level, table[variable_index].address);

		} 

		// move to next token
		token_index++;
	
	}
	
	// else if current token == number
	else if(tokens[token_index].type == number){

		// emit LIT, m = number_value
		emit(LIT, 0, tokens[token_index].number_value);

		// move to next token
		token_index++;

	}

	// else
	else {

		// error 19, return
		print_parser_error(19, 0);

		// set error flag to -1
		error = -1;

		// return
		return;
	}

	//printf("end of factor\n");

	// END OF FACTOR()
}

// adds a new instruction to the end of the code
void emit(int op, int l, int m)
{
	code[code_index].op = op;
	code[code_index].l = l;
	code[code_index].m = m;
	code_index++;
}

// adds a new symbol to the end of the table
void add_symbol(int kind, char name[], int value, int level, int address)
{
	table[table_index].kind = kind;
	strcpy(table[table_index].name, name);
	table[table_index].value = value;
	table[table_index].level = level;
	table[table_index].address = address;
	table[table_index].mark = 0;
	table_index++;
}

// marks all of the current procedure's symbols
void mark()
{
	int i;
	for (i = table_index - 1; i >= 0; i--)
	{
		if (table[i].mark == 1)
			continue;
		if (table[i].level < level)
			return;
		table[i].mark = 1;
	}
}

// returns -1 if there are no other symbols with the same name within this procedure
int multiple_declaration_check(char name[])
{
	int i;
	for (i = 0; i < table_index; i++)
		if (table[i].mark == 0 && table[i].level == level && strcmp(name, table[i].name) == 0)
			return i;
	return -1;
}

// returns the index of the symbol with the desired name and kind, prioritizing 
// 		symbols with level closer to the current level
int find_symbol(char name[], int kind)
{
	int i;
	int max_idx = -1;
	int max_lvl = -1;
	for (i = 0; i < table_index; i++)
	{
		if (table[i].mark == 0 && table[i].kind == kind && strcmp(name, table[i].name) == 0)
		{
			if (max_idx == -1 || table[i].level > max_lvl)
			{
				max_idx = i;
				max_lvl = table[i].level;
			}
		}
	}
	return max_idx;
}

void print_parser_error(int error_code, int case_code)
{
	switch (error_code)
	{
		case 1 :
			printf("Parser Error 1: missing . \n");
			break;
		case 2 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 2: missing identifier after keyword const\n");
					break;
				case 2 :
					printf("Parser Error 2: missing identifier after keyword var\n");
					break;
				case 3 :
					printf("Parser Error 2: missing identifier after keyword procedure\n");
					break;
				case 4 :
					printf("Parser Error 2: missing identifier after keyword call\n");
					break;
				case 5 :
					printf("Parser Error 2: missing identifier after keyword read\n");
					break;
				case 6 :
					printf("Parser Error 2: missing identifier after keyword def\n");
					break;
				default :
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 3 :
			printf("Parser Error 3: identifier is declared multiple times by a procedure\n");
			break;
		case 4 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 4: missing := in constant declaration\n");
					break;
				case 2 :
					printf("Parser Error 4: missing := in assignment statement\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 5 :
			printf("Parser Error 5: missing number in constant declaration\n");
			break;
		case 6 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 6: missing ; after constant declaration\n");
					break;
				case 2 :
					printf("Parser Error 6: missing ; after variable declaration\n");
					break;
				case 3 :
					printf("Parser Error 6: missing ; after statement in begin-end\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 7 :
			printf("Parser Error 7: procedures and constants cannot be assigned to\n");
			break;
		case 8 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 8: undeclared identifier used in assignment statement\n");
					break;
				case 2 :
					printf("Parser Error 8: undeclared identifier used in call statement\n");
					break;
				case 3 :
					printf("Parser Error 8: undeclared identifier used in read statement\n");
					break;
				case 4 :
					printf("Parser Error 8: undeclared identifier used in arithmetic expression\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 9 :
			printf("Parser Error 9: variables and constants cannot be called\n");
			break;
		case 10 :
			printf("Parser Error 10: begin must be followed by end\n");
			break;
		case 11 :
			printf("Parser Error 11: if must be followed by then\n");
			break;
		case 12 :
			printf("Parser Error 12: while must be followed by do\n");
			break;
		case 13 :
			printf("Parser Error 13: procedures and constants cannot be read\n");
			break;
		case 14 :
			printf("Parser Error 14: missing {\n");
			break;
		case 15 :
			printf("Parser Error 15: { must be followed by }\n");
			break;
		case 16 :
			printf("Parser Error 16: missing relational operator\n");
			break;
		case 17 :
			printf("Parser Error 17: procedures cannot be used in arithmetic\n");
			break;
		case 18 :
			printf("Parser Error 18: ( must be followed by )\n");
			break;
		case 19 :
			printf("Parser Error 19: invalid expression\n");
			break;
		default:
			printf("Implementation Error: unrecognized error code\n");

	}
}

void print_assembly_code()
{
	int i;
	printf("Assembly Code:\n");
	printf("Line\tOP Code\tOP Name\tL\tM\n");
	for (i = 0; i < code_index; i++)
	{
		printf("%d\t%d\t", i, code[i].op);
		switch(code[i].op)
		{
			case LIT :
				printf("LIT\t");
				break;
			case OPR :
				switch (code[i].m)
				{
					case RTN :
						printf("RTN\t");
						break;
					case ADD : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("ADD\t");
						break;
					case SUB : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("SUB\t");
						break;
					case MUL : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("MUL\t");
						break;
					case DIV : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("DIV\t");
						break;
					case EQL : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("EQL\t");
						break;
					case NEQ : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("NEQ\t");
						break;
					case LSS : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("LSS\t");
						break;
					case LEQ : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("LEQ\t");
						break;
					case GTR : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("GTR\t");
						break;
					case GEQ : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("GEQ\t");
						break;
					default :
						printf("err\t");
						break;
				}
				break;
			case LOD :
				printf("LOD\t");
				break;
			case STO :
				printf("STO\t");
				break;
			case CAL :
				printf("CAL\t");
				break;
			case INC :
				printf("INC\t");
				break;
			case JMP :
				printf("JMP\t");
				break;
			case JPC : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
				printf("JPC\t");
				break;
			case SYS :
				switch (code[i].m)
				{
					case WRT : // DO NOT ATTEMPT TO IMPLEMENT THIS, YOU WILL GET A ZERO IF YOU DO
						printf("WRT\t");
						break;
					case RED :
						printf("RED\t");
						break;
					case HLT :
						printf("HLT\t");
						break;
					default :
						printf("err\t");
						break;
				}
				break;
			default :
				printf("err\t");
				break;
		}
		printf("%d\t%d\n", code[i].l, code[i].m);
	}
	printf("\n");
}

void print_symbol_table()
{
	int i;
	printf("Symbol Table:\n");
	printf("Kind | Name        | Value | Level | Address | Mark\n");
	printf("---------------------------------------------------\n");
	for (i = 0; i < table_index; i++)
		printf("%4d | %11s | %5d | %5d | %5d | %5d\n", table[i].kind, table[i].name, table[i].value, table[i].level, table[i].address, table[i].mark); 
	printf("\n");
}