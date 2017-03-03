#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

//@stag globals
//data
char *g_data_stream, 	//data of the file being currently compiled
	 *g_stream_it,		//iterator on the file data
	 *g_data,			//pointer to the base of program data block
	 *g_data_it;		//an iterator over the data


//compiler global variables
int g_currentToken, 	//current scanned token
	g_tokenValue, 		//current token value
	g_validToken, 		//flag if the valid token is valid
	g_currentLine, 		//current line of the file
	g_currentType, 		//current expression type
	g_localScope, 		//local variable offset
	g_tokenHash, 		//tmp variable that holds the current token hash
	*g_symbolTable,		//this pointer holds the entire symbol table
	*g_currentID, 		//this pointer holds the symbol table entry of the current scanned id
	*g_byteCode, 		//holds the emitted byte code
	*g_byteCode_it;		//an iterator over the emitted byte code


//@stag TOKEN
//tokens supported by the compiler
enum
{
	TOKEN_NUM = 128,
	TOKEN_CHAR,
	TOKEN_ELSE,
	TOKEN_ENUM,
	TOKEN_IF,
	TOKEN_INT,
	TOKEN_RETURN,
	TOKEN_SIZEOF,
	TOKEN_WHILE,
	TOKEN_ID,
	TOKEN_STR,
	TOKEN_SCOMMENT,
	TOKEN_ASSIGN,
	TOKEN_COND,
	TOKEN_LOR,
	TOKEN_LAND,
	TOKEN_OR,
	TOKEN_XOR,
	TOKEN_AND,
	TOKEN_EQ,
	TOKEN_NEQ,
	TOKEN_LE,
	TOKEN_GR,
	TOKEN_LEQ,
	TOKEN_GEQ,
	TOKEN_SHL,
	TOKEN_SHR,
	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_MOD,
	TOKEN_DEC,
	TOKEN_INC,
	TOKEN_BRACKET
};

//@stag TYPE
//types supported by the compiler
enum
{
	TYPE_CHAR,
	TYPE_INT,
	TYPE_PTR
};

//@stag INS
//byte code instructions
enum
{
	//instructions
	INS_ENTER = 1,//enters a subroutine
	INS_LEAVE,		//leaves a subroutine
	INS_BZ, 		//branch if zero
	INS_BNZ, 		//branch if not zero
	INS_OR, 		//or
	INS_XOR, 		//xor
	INS_AND, 		//and
	INS_NEQ,		//not equal
	INS_LE, 		//less than
	INS_GR, 		//greater than
	INS_LEQ, 		//less than or equal
	INS_GEQ, 		//greater than or equal
	INS_SHL, 		//shift left
	INS_SHR, 		//shift right
	INS_DIV, 		//divide
	INS_MOD, 		//modulus
	INS_IMM, 		//immediate value
	INS_PSH, 		//pushes the stack
	INS_ADJ, 		//adjusts the stack
	INS_JSR,		//jump to subroutine
	INS_JMP, 		//jumps indefinitly
	INS_LEA, 		//load effective address
	INS_LOADCHAR, 	//instructs vm to load a char size
	INS_LOADINT, 	//instructs vm to load an int size
	INS_EQ, 		//equal
	INS_MUL, 		//performs multiplication
	INS_ADD, 		//performs addition
	INS_SUB, 		//performs subtraction
	INS_SAVECHAR, 	//instructs vm to save a char size
	INS_SAVEINT, 	//instructs vm to save an int size
	INS_NOINS, 		//no instruction
	//libc stuff
	INS_OPEN,
	INS_READ,
	INS_CLOSE,
	INS_PRINTF,
	INS_MALLOC,
	INS_MEMSET,
	INS_MEMCMP,
	INS_MEMCPY,
	INS_MMAP,
	INS_DLSYM,
	INS_QSORT,
	INS_EXIT,
	INS_VOID,
	INS_MAIN
};

//@stag CLASS
//classes of the symbol table entry
enum
{
	CLASS_SYSTEM = 1,
	CLASS_FUNNCTION,
	CLASS_GLOBAL,
	CLASS_LOCAL,
	CLASS_ID,
	CLASS_NUM
};

//@stag SYM
//since we have no structs then we make offsets in the memory for our data
//symbol struct offset
enum {
	SYM_TOKEN = 0,		//holds the token enum
	SYM_HASH = 1,		//hash of the name for speed compare
	SYM_NAME = 2,		//ptr to the name of the thing whatever it is
	SYM_NSIZE = 3,		//size of the name string
	SYM_CLASS = 4,		//class of this entry. ie, system, function, global ...
	SYM_TYPE = 5,		//type of this entry. ie, variable type, function type
	SYM_VALUE = 6,		//value of this entry. ie, enum value
	SYM_TCLASS = 7, 	//holds the prev class in case of shadowing
	SYM_TTYPE = 8, 		//holds the prev type in case of shadowing
	SYM_TVALUE = 9,  	//holds the prev value in case of shadowing
	SYM_SIZE = 10		//holds the size of the struct for malloc purposes
};

//procedure declaration

//@stag symExist

//checks if the symbol indicated by the position of the @g_stream_it and the gived size as arg is listed inside the symbol table
//if the identifier doesn't exist then it will return 0 else it will return a pointer to it's location in the symbol table
int*
symExist(int token_size, int creation_flag)
{
	//put the cursor to the start of the symbol table
	int* tmp;
	tmp = g_symbolTable;

	//while no zeros then we are in bounds. remember the memset?
	while(tmp[SYM_TOKEN])
	{
		//identify the token
		//first through the hash for easy rejection then test the name of the token
		if(g_tokenHash == tmp[SYM_HASH] && !memcmp((char *)tmp[SYM_NAME], g_stream_it, token_size))
		{
			//symbol table entry is found then return it
			return tmp;
		}
		tmp = tmp + SYM_SIZE;
	}

	//symbol not found then create a new one if the flag for creation is on
	if(creation_flag)
	{
		//@future: if the symbol table ended then you can create a bigger one and copy everything to it
		tmp[SYM_TOKEN] = 0;
		tmp[SYM_HASH] = 0;
		tmp[SYM_NAME] = 0;
		tmp[SYM_NAME] = 0;
		tmp[SYM_NSIZE] = 0;
		tmp[SYM_CLASS] = 0;
		tmp[SYM_TYPE] = 0;
		tmp[SYM_VALUE] = 0;
		tmp[SYM_TCLASS] = 0;
		tmp[SYM_TTYPE] = 0;
		tmp[SYM_TVALUE] = 0;
		return tmp;
	}

	//symbol not found then return 0
	return 0;
}

//@stag nextToken

//scans the next token and outputs result to @g_currentTokens
void 
nextToken()
{
	char *tmp, c, *data_it;

	int token_size, *symbol_entry;

	while(*g_stream_it != 0)
	{	
		c = *g_stream_it;
		//check for new line to inc the line number
		if(c == '\n')
		{
			++g_currentLine;
		}
		//ignore the includes
		else if(c == '#')
		{
			//while not eof and not new line then consume the char
			while(*g_stream_it != 0 && *g_stream_it != '\n')
				++g_stream_it;
		}
		//identifiers goes here
		else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
		{
			//get to the end of the identifier token
			tmp = g_stream_it;

			//start the hashing and scanning operation
			g_tokenHash = *tmp;
			while((*tmp >= 'a' && *tmp <= 'z') || (*tmp >= '0' && *tmp <= '9') || (*tmp >= 'A' && *tmp <= 'Z') || *tmp == '_')
			{
				g_tokenHash = g_tokenHash * 147 + *tmp;
				tmp++;
			}

			//finalize computing the hash and the identifier size;
			token_size = tmp-g_stream_it;
			g_tokenHash = (g_tokenHash << 6) + token_size;

			//check if in the symbol table if not then create a new entry and return it
			symbol_entry = symExist(token_size, 1);

			//assign the current token to the symbol entry 
			//if the returned entry is newly created
			if(symbol_entry[SYM_TOKEN] == 0)
			{
				g_currentToken = symbol_entry[SYM_TOKEN] = TOKEN_ID;
				symbol_entry[SYM_HASH] = g_tokenHash;
				symbol_entry[SYM_NAME] = (int)g_stream_it;
				symbol_entry[SYM_NSIZE] = token_size;
			}
			//if found before
			else
			{
				g_currentToken = symbol_entry[SYM_TOKEN];
			}
			g_currentID = symbol_entry;
			g_validToken = 1;
			//before we return we must advance the global iterator
			g_stream_it = tmp;
			return;

		}
		//numeric constants goes here
		else if(c >= '0' && c <= '9')
		{
			tmp = g_stream_it+1;

			//starts parsing the decimal constants
			if(g_tokenValue = c - '0') 	//this if will be nonzero if the frist digit is non zero thus resulting in the correct behaviour
			{
				while(*tmp >= '0' && *tmp <= '9')
				{
					//shift in decimal representation
					g_tokenValue = g_tokenValue * 10;
					//append the digit
					g_tokenValue = g_tokenValue + (*tmp - '0');

					//move the cursor
					++tmp;
				}
			}
			else if(*tmp == 'x' || *tmp == 'X') //if starting with a zero then it might be a hes digit
			{
				while((*tmp >= '0' && *tmp <= '9') || (*tmp >= 'a' && *tmp <= 'f') || (*tmp >= 'A' && *tmp <= 'F'))
				{
					//shift in hex representation
					//get the offset above 9 if the char is [a-fA-F] the expr (*tmp&15) will result the following
					// a = 1, A = 1
					// b = 2, B = 2
					// c = 3, C = 3
					// d = 4, D = 4
					// e = 5, E = 5
					// f = 6, F = 6
					//this is a property due to ascii representation of this letters
					//if it's greater than 9 then add the 9 since the and will only get the reminder of the 16
					//15 in decimal = 1111 in binary
					g_tokenValue = (g_tokenValue * 16) + (*tmp & 15) + (*tmp >= 'A' ? 9 : 0);
					//advance the iterator
					++tmp;
				}
			}
			else 	//else assume that it's a octal digit
			{
				while(*tmp >= '0' && *tmp <= '7')
				{
					g_tokenValue = g_tokenValue * 8 + *tmp - '0';
					++tmp;
				}
			}

			//assign the current token to number
			g_currentToken = TOKEN_NUM;
			g_validToken = 1;
			//before the return we must advance the global iterator
			g_stream_it = tmp;
			return;
		}
		//comments go here
		else if(c == '/')
		{
			tmp = g_stream_it+1;
			//single line comment
			if(*tmp == '/')
			{
				while(*tmp != 0 && *tmp != '\n')
					++tmp;
				// g_currentToken = TOKEN_SCOMMENT;
				// g_validToken = 1;
				g_stream_it = tmp;
				//continue to parse another token
			}
			//division operator
			else
			{
				g_currentToken = TOKEN_DIV;
				g_validToken = 1;
				g_stream_it = tmp;
				return;
			}
		}
		//string literals
		else if(c == '\'' || c == '"')
		{
			data_it = g_data_it;
			tmp = g_stream_it+1;
			while(*tmp != 0 && *tmp != c)
			{
				//handle special chars
				if(*tmp == '\\')
				{
					if(*(tmp+1) != 0 && *(tmp+1) == 'n')
					{
						g_tokenValue = '\n';
						tmp++;
					}
				}
				//normal char
				else
				{
					g_tokenValue = *tmp;
				}

				*data_it++ = g_tokenValue;
				tmp++;
			}

			//end the string
			*data_it++ = '\0';
			g_stream_it = tmp+1;
			g_tokenValue = (int)g_data_it;
			g_currentToken = TOKEN_STR;
			g_validToken = 1;
			g_data_it = data_it;
			return;
		}
		//small tokens
		//equal and assign
		else if(c == '=')
		{
			tmp = g_stream_it+1;
			if(*tmp == '=')
			{
				g_stream_it = tmp+1;
				g_validToken = 1;
				g_currentToken = TOKEN_EQ;
				return;
			}

			g_currentToken = TOKEN_ASSIGN;
			g_validToken = 1;
			g_stream_it++;
			return;
		}
		//plus and increment
		else if(c == '+')
		{
			tmp = g_stream_it+1;
			if(*tmp == '+')
			{
				g_stream_it = tmp+1;
				g_validToken = 1;
				g_currentToken = TOKEN_INC;
				return;
			}

			g_currentToken = TOKEN_ADD;
			g_validToken = 1;
			g_stream_it++;
			return;
		}
		//minus and decrement
		else if(c == '-')
		{
			tmp = g_stream_it+1;
			if(*tmp == '-')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_DEC;
				g_validToken = 1;
				return;
			}

			g_currentToken = TOKEN_SUB;
			g_validToken = 1;
			g_stream_it++;
			return;
		}
		//not equal
		else if(c == '!')
		{
			tmp = g_stream_it+1;
			if(*tmp == '=')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_NEQ;
				g_validToken = 1;
				return;
			}
			g_validToken = 0;
			return;
		}
		//less than, less than or euqal and shift left
		else if(c == '<')
		{
			tmp = g_stream_it+1;
			//less than or equal
			if(*tmp == '=')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_LEQ;
				g_validToken = 1;
				return;
			}
			//shift left
			else if(*tmp == '<')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_SHL;
				g_validToken = 1;
				return;
			}

			//less than
			g_stream_it++;
			g_currentToken = TOKEN_LE;
			g_validToken = 1;
			return;
		}
		//greater than, greater than or equal and right shift
		else if(c == '>')
		{
			tmp = g_stream_it+1;
			//greater than or equal
			if(*tmp == '=')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_GEQ;
				g_validToken = 1;
				return;
			}
			//shift right
			else if(*tmp == '>')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_SHR;
				g_validToken = 1;
				return;
			}

			//greater than
			g_stream_it++;
			g_currentToken = TOKEN_GR;
			g_validToken = 1;
			return;
		}
		//logical or and or
		else if(c == '|')
		{
			tmp = g_stream_it+1;
			if(*tmp == '|')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_LOR;
				g_validToken = 1;
				return;
			}

			g_stream_it++;
			g_currentToken = TOKEN_OR;
			g_validToken = 1;
			return;
		}
		//logical and and and
		else if(c == '&')
		{
			tmp = g_stream_it+1;
			if(*tmp == '&')
			{
				g_stream_it = tmp+1;
				g_currentToken = TOKEN_LAND;
				g_validToken = 1;
				return;
			}

			g_stream_it++;
			g_currentToken = TOKEN_AND;
			g_validToken = 1;
			return;
		}
		//xor
		else if(c == '^')
		{
			g_stream_it++;
			g_currentToken = TOKEN_XOR;
			g_validToken = 1;
			return;
		}
		//mod
		else if(c == '%')
		{
			g_stream_it++;
			g_currentToken = TOKEN_MOD;
			g_validToken = 1;
			return;
		}
		//mul
		else if(c == '*')
		{
			g_stream_it++;
			g_currentToken = TOKEN_MUL;
			g_validToken = 1;
			return;
		}
		//bracket
		else if(c == '[')
		{
			g_stream_it++;
			g_currentToken = TOKEN_BRACKET;
			g_validToken = 1;
			return;
		}
		//cond
		else if(c == '?')
		{
			g_stream_it++;
			g_currentToken = TOKEN_COND;
			g_validToken = 1;
			return;
		}
		//pass these tokens as is
		else if(c == '~' ||
				c == ';' ||
				c == '{' ||
				c == '}' ||
				c == '(' ||
				c == ')' ||
				c == ']' ||
				c == ',' ||
				c == ':')
		{
			g_currentToken = c;
			g_stream_it++;
			g_validToken = 1;
			return;
		}


		//move the position
		++g_stream_it;
	}

	g_validToken = 0;
	return;
}

//given the token id this function returns the name of the token

//@stag tokenName
char*
tokenName(int token_id)
{
	if(token_id == TOKEN_ID)
		return "identifier";
	if(token_id == TOKEN_STR)
		return "string";
	if(token_id == TOKEN_DIV)
		return "divide";
	if(token_id == TOKEN_SCOMMENT)
		return "single-line comment";
	if(token_id == TOKEN_NUM)
		return "number";
	if(token_id == TOKEN_EQ)
		return "equal";
	if(token_id == TOKEN_ASSIGN)
		return "assign";
	if(token_id == TOKEN_INC)
		return "increment";
	if(token_id == TOKEN_ADD)
		return "add";
	if(token_id == TOKEN_SUB)
		return "sub";
	if(token_id == TOKEN_DEC)
		return "decrement";
	if(token_id == TOKEN_NEQ)
		return "not equal";
	if(token_id == TOKEN_LEQ)
		return "less equal";
	if(token_id == TOKEN_LE)
		return "less";
	if(token_id == TOKEN_SHL)
		return "shift left";
	if(token_id == TOKEN_GR)
		return "greater";
	if(token_id == TOKEN_GEQ)
		return "greater equal";
	if(token_id == TOKEN_SHR)
		return "shift right";
	if(token_id == TOKEN_LOR)
		return "logical or";
	if(token_id == TOKEN_OR)
		return "or";
	if(token_id == TOKEN_LAND)
		return "logical and";
	if(token_id == TOKEN_AND)
		return "and";
	if(token_id == TOKEN_XOR)
		return "xor";
	if(token_id == TOKEN_MOD)
		return "mod";
	if(token_id == TOKEN_MUL)
		return "mul";
	if(token_id == TOKEN_BRACKET)
		return "bracket";
	if(token_id == TOKEN_COND)
		return "cond";
	if(token_id == TOKEN_CHAR)
		return "char";
	if(token_id == TOKEN_ELSE)
		return "else";
	if(token_id == TOKEN_ENUM)
		return "enum";
	if(token_id == TOKEN_IF)
		return "if";
	if(token_id == TOKEN_INT)
		return "int";
	if(token_id == TOKEN_RETURN)
		return "return";
	if(token_id == TOKEN_SIZEOF)
		return "sizeof";
	if(token_id == TOKEN_WHILE)
		return "while";


	return &g_currentToken;
}

//@stag className
char*
className(int class_id)
{
	if(class_id == CLASS_LOCAL)
		return "local";
	if(class_id == CLASS_SYSTEM)
		return "system";
	if(class_id == CLASS_NUM)
		return "num";
	if(class_id == CLASS_FUNNCTION)
		return "function";
	if(class_id == CLASS_GLOBAL)
		return "global";
	if(class_id == CLASS_ID)
		return "id";

	return "unidentified";
}

//@stag insName
char*
insName(int ins_id)
{
	if(ins_id == INS_ENTER)
		return "ENTER";
	if(ins_id == INS_LEAVE)
		return "LEAVE";
	if(ins_id == INS_BZ)
		return "BZ";
	if(ins_id == INS_BNZ)
		return "BNZ";
	if(ins_id == INS_OR)
		return "OR";
	if(ins_id == INS_XOR)
		return "XOR";
	if(ins_id == INS_AND)
		return "AND";
	if(ins_id == INS_NEQ)
		return "NEQ";
	if(ins_id == INS_LE)
		return "LE";
	if(ins_id == INS_GR)
		return "GR";
	if(ins_id == INS_LEQ)
		return "LEQ";
	if(ins_id == INS_GEQ)
		return "GEQ";
	if(ins_id == INS_SHL)
		return "SHL";
	if(ins_id == INS_SHR)
		return "SHR";
	if(ins_id == INS_DIV)
		return "DIV";
	if(ins_id == INS_MOD)
		return "MOD";

	if(ins_id == INS_IMM)
		return "IMM";
	if(ins_id == INS_PSH)
		return "PSH";
	if(ins_id == INS_ADJ)
		return "ADJ";
	if(ins_id == INS_JSR)
		return "JSR";
	if(ins_id == INS_JMP)
		return "JMP";
	if(ins_id == INS_LEA)
		return "LEA";
	if(ins_id == INS_LOADCHAR)
		return "LOADCHAR";
	if(ins_id == INS_LOADINT)
		return "LOADINT";
	if(ins_id == INS_EQ)
		return "EQ";
	if(ins_id == INS_MUL)
		return "MUL";
	if(ins_id == INS_ADD)
		return "ADD";
	if(ins_id == INS_SUB)
		return "SUB";
	if(ins_id == INS_SAVECHAR)
		return "SAVECHAR";
	if(ins_id == INS_SAVEINT)
		return "SAVEINT";
	if(ins_id == INS_OPEN)
		return "OPEN";
	if(ins_id == INS_READ)
		return "READ";
	if(ins_id == INS_CLOSE)
		return "CLOSE";
	if(ins_id == INS_PRINTF)
		return "PRINTF";
	if(ins_id == INS_MALLOC)
		return "MALLOC";
	if(ins_id == INS_MEMSET)
		return "MEMSET";
	if(ins_id == INS_MEMCMP)
		return "MEMCMP";
	if(ins_id == INS_MEMCPY)
		return "MEMCPY";
	if(ins_id == INS_MMAP)
		return "MMAP";
	if(ins_id == INS_DLSYM)
		return "DLSYM";
	if(ins_id == INS_QSORT)
		return "QSORT";
	if(ins_id == INS_EXIT)
		return "EXIT";
	if(ins_id == INS_VOID)
		return "VOID";
	if(ins_id == INS_MAIN)
		return "MAIN";
	if(ins_id == INS_NOINS)
		return "NOINS";

	return 0;
}

//prints the current token
//@stag printCurrentToken
void
printCurrentToken()
{
	if(g_currentToken == TOKEN_STR)
		printf("current Token = %d: %s, str = %s\n", g_currentToken, tokenName(g_currentToken), g_data_it);
	else if(g_currentToken == TOKEN_ID)
	{
		//printf("current Token = %d: %s, id = %.*s\n", g_currentToken, tokenName(g_currentToken), g_currentID[SYM_NSIZE], g_currentID[SYM_NAME]);
		printf("current Token = %d: %s, id = %.*s\n", g_currentID[SYM_TOKEN], tokenName(g_currentID[SYM_TOKEN]), g_currentID[SYM_NSIZE], g_currentID[SYM_NAME]);
	}
	else
		printf("current Token = %d: %s\n", g_currentToken, tokenName(g_currentToken));

	return;
}

//prints the entire symbol table
//@stag printSymbolTable
void
printSymbolTable()
{
	//start from the first entry
	int *tmp;
	int i;

	tmp = g_symbolTable;

	i = 0;
	//while inside the boundaries of the table
	while(*tmp != 0)
	{
		printf("entry #%d: token = [%d, %s], ", i, tmp[SYM_TOKEN], tokenName(tmp[SYM_TOKEN]));
		printf("name = %.*s, value = %d, type = %d, class = %s\n", tmp[SYM_NSIZE], tmp[SYM_NAME], tmp[SYM_VALUE], tmp[SYM_TYPE], className(tmp[SYM_CLASS]));
		tmp = tmp + SYM_SIZE;
		i++;
	}
}

//@stag printByteCode
void
printByteCode()
{
	int i, *bytecode_it;
	char* ins_name;

	bytecode_it = g_byteCode;
	i=1;

	printf("BYTE_CODE_START\n");
	while((bytecode_it < g_byteCode+256 * 1024))
	{
		ins_name = insName(*bytecode_it);
		if(ins_name)
			printf("%d: %s\n", i++, ins_name);
		else
			printf("%d: %d\n", i++, *bytecode_it);
		bytecode_it++;

	}
	printf("BYTE_CODE_END");
}

//parses an expr
//@stag expr
void
expr(int level)
{
	int *symbol_tmp, counter;

	if(g_currentToken == 0)
	{
		printf("%d: unexpected eof in expression\n", g_currentLine);
		exit(-1);
	}
	//if it's a number
	else if(g_currentToken == TOKEN_NUM)
	{
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = g_tokenValue;
		nextToken();
		g_currentType = TYPE_INT;
	}
	//if it's a string
	else if(g_currentToken == TOKEN_STR)
	{
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = g_tokenValue;
		nextToken();
		g_currentType = TYPE_PTR;
	}
	//size of
	else if(g_currentToken == TOKEN_SIZEOF)
	{
		nextToken();
		//checks for the open paren
		if(g_currentToken == '(')
			nextToken();
		else
		{
			printf("%d: open paren expected in sizeof\n", g_currentLine);
			exit(-1);
		}

		//checks for the type
		g_currentType = TYPE_INT;
		if(g_currentToken == TOKEN_INT)
			nextToken();
		else if(g_currentToken == TOKEN_CHAR)
		{
			nextToken();
			g_currentType = TYPE_CHAR;
		}

		while(g_currentToken == TOKEN_MUL)
		{
			nextToken();
			g_currentType = g_currentType + TYPE_PTR;
		}

		//checks for close paren
		if(g_currentToken == ')')
			nextToken();
		else
		{
			printf("%d: close paren expected in sizeof\n", g_currentLine);
			exit(-1);
		}

		//emit the byte code
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = (g_currentType == TYPE_CHAR) ? sizeof(char) : sizeof(int);

		g_currentType = TYPE_INT;
	}
	//identifier
	else if(g_currentToken == TOKEN_ID)
	{
		symbol_tmp = g_currentID;
		nextToken();

		//this is a function call
		if(g_currentToken == '(')
		{
			counter = 0;
			nextToken();
			//parse function args
			while(g_currentToken != ')')
			{
				expr(TOKEN_ASSIGN);

				//pushs an item into the stack
				*++g_byteCode_it = INS_PSH;
				++counter;

				if(g_currentToken == ',')
					nextToken();
			}

			nextToken();

			//if it's sys call not user defined
			if(symbol_tmp[SYM_CLASS] == CLASS_SYSTEM)
				*++g_byteCode_it = symbol_tmp[SYM_VALUE];
			//if it's a function
			else if(symbol_tmp[SYM_CLASS] == CLASS_FUNNCTION)
			{
				//jump to function
				*++g_byteCode_it = INS_JSR;
				*++g_byteCode_it = symbol_tmp[SYM_VALUE];
				//printf("jsr function ptr: %d\n", symbol_tmp[SYM_VALUE]);
			}
			else
			{
				printf("%d: bad function call\n", g_currentLine);
				exit(-1);
			}

			//function call has arguments then we need to adjust the stack
			if(counter)
			{
				*++g_byteCode_it = INS_ADJ;
				*++g_byteCode_it = counter;
			}

			g_currentType = symbol_tmp[SYM_TYPE];
		}
		//if it's an enum
		else if(symbol_tmp[SYM_CLASS] == CLASS_NUM)
		{
			*++g_byteCode_it = INS_IMM;
			*++g_byteCode_it = symbol_tmp[SYM_VALUE];
			g_currentType = TYPE_INT;
		}
		//then it must be a plain variable we need to subtitue it's value or load it
		else
		{	
			//check local scope
			if(symbol_tmp[SYM_CLASS] == CLASS_LOCAL)
			{
				//load this variable
				*++g_byteCode_it = INS_LEA;
				*++g_byteCode_it = g_localScope - symbol_tmp[SYM_VALUE];
			}
			//check global scope
			else if(symbol_tmp[SYM_CLASS] == CLASS_GLOBAL)
			{
				*++g_byteCode_it = INS_IMM;
				*++g_byteCode_it = symbol_tmp[SYM_VALUE];
			}
			//this variable is not defined
			else
			{
				printf("%d: undefined variable\n", g_currentLine);
				exit(-1);
			}
			//indicator to load int or char
			*++g_byteCode_it = ((g_currentType = symbol_tmp[SYM_TYPE]) == TYPE_CHAR) ? INS_LOADCHAR : INS_LOADINT;
		}
	}
	//if it's a open paren "(expr)"
	else if(g_currentToken == '(')
	{
		nextToken();
		//check if it's a cast (type)
		if(g_currentToken == TOKEN_INT || g_currentToken == TOKEN_CHAR)
		{
			counter = (g_currentToken == TOKEN_INT) ? TYPE_INT : TYPE_CHAR;
			nextToken();
			while(g_currentToken == TOKEN_MUL)
			{
				nextToken();
				counter = counter + TYPE_PTR;
			}

			if(g_currentToken == ')')
				nextToken();
			else
			{
				printf("%d: bad cast\n", g_currentLine);
				exit(-1);
			}

			expr(TOKEN_INC);
			g_currentType = counter;
		}
		//if it's just an expr
		else
		{
			expr(TOKEN_ASSIGN);
			if(g_currentToken == ')')
				nextToken();
			else
			{
				printf("%d: close paren expected\n", g_currentLine);
				exit(-1);
			}
		}
	}
	//dereference
	else if(g_currentToken == TOKEN_MUL)
	{
		nextToken();
		expr(TOKEN_INC);
		if(g_currentToken > TYPE_INT)
			g_currentType = g_currentType - TYPE_PTR;
		else
		{
			printf("%d: bad dereference\n", g_currentLine);
			exit(-1);
		}

		*++g_byteCode_it = (g_currentType == TYPE_CHAR) ? INS_LOADCHAR : INS_LOADINT;
	}
	//reference
	else if(g_currentToken == TOKEN_AND)
	{
		nextToken();
		expr(TOKEN_INC);
		if(*g_byteCode_it == INS_LOADCHAR || *g_byteCode_it == INS_LOADINT)
			--g_byteCode_it;
		else
		{
			printf("%d: bad address\n", g_currentLine);
			exit(-1);
		}
		g_currentType = g_currentType + TYPE_PTR;
	}
	//not expr
	else if(g_currentToken == '!')
	{
		nextToken();
		expr(TOKEN_INC);
		*++g_byteCode_it = INS_PSH;
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = 0;
		*++g_byteCode_it = INS_EQ;
		g_currentType = TYPE_INT;
	}
	//logical not expr
	else if(g_currentToken == '~')
	{
		nextToken();
		expr(TOKEN_INC);
		*++g_byteCode_it = INS_PSH;
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = -1;
		*++g_byteCode_it = INS_EQ;
		g_currentType = TYPE_INT;
	}
	//addition expr
	else if(g_currentToken == TOKEN_ADD)
	{
		nextToken();
		expr(TOKEN_INC);
		g_currentType = TYPE_INT;
	}
	//subtration expr
	else if(g_currentToken == TOKEN_SUB)
	{
		nextToken();
		*++g_byteCode_it = INS_IMM;
		if(g_currentToken == TOKEN_NUM)
		{
			*++g_byteCode_it = -g_tokenValue;
			nextToken();
		}
		else
		{
			*++g_byteCode_it = -1;
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_INC);
			*++g_byteCode_it = INS_MUL;
		}
		g_currentType = TYPE_INT;
	}
	//increment or decrement
	else if(g_currentToken == TOKEN_INC || g_currentToken == TOKEN_DEC)
	{
		//here i use the counter as tmp
		counter = g_currentToken;

		nextToken();
		expr(TOKEN_INC);

		//this is a preincrement of decrement
		if(*g_byteCode_it == INS_LOADCHAR)
		{
			*g_byteCode_it = INS_PSH;
			*++g_byteCode_it = INS_LOADCHAR;
		}
		else if(*g_byteCode_it == INS_LOADINT)
		{
			*g_byteCode_it = INS_PSH;
			*++g_byteCode_it = INS_LOADINT;	
		}
		else
		{
			printf("%d: bad lvalue in pre-increment\n", g_currentLine);
			exit(-1);
		}
		*++g_byteCode_it = INS_PSH;
		*++g_byteCode_it = INS_IMM;
		*++g_byteCode_it = (g_currentType > TYPE_PTR) ? sizeof(int) : sizeof(char);
		*++g_byteCode_it = (counter == TOKEN_INC) ? INS_ADD : INS_SUB;
		*++g_byteCode_it = (g_currentType == TYPE_CHAR) ? INS_SAVECHAR : INS_SAVEINT;
	}
	//this the end of the expressions
	else
	{
		printf("%d: bad expression\n", g_currentLine);
		printCurrentToken();
		exit(-1);
	}


	//precedence climbing
	while(g_currentToken >= level)
	{
		counter = g_currentType;

		//if operator being parsed is =
		if(g_currentToken == TOKEN_ASSIGN)
		{
			nextToken();
			if(*g_byteCode_it == INS_LOADCHAR || *g_byteCode_it == INS_LOADINT)
				*g_byteCode_it = INS_PSH;
			else
			{
				printf("%d: bad lvalue in assignment\n", g_currentLine);
				exit(-1);
			}
			expr(TOKEN_ASSIGN);
			*++g_byteCode_it = ((g_currentType = counter) == TYPE_CHAR) ? INS_SAVECHAR : INS_SAVEINT;
		}
		//conditional expression ?
		else if(g_currentToken == TOKEN_COND)
		{
			nextToken();
			*++g_byteCode_it = INS_BZ;
			symbol_tmp = ++g_byteCode_it;

			expr(TOKEN_ASSIGN);

			if(g_currentToken == ':')
				nextToken();
			else
			{
				printf("%d: conditional missing colon\n", g_currentLine);
				exit(-1);
			}

			*symbol_tmp = (int)(g_byteCode_it+3);
			*++g_byteCode_it = INS_JMP;
			symbol_tmp = ++g_byteCode_it;
			expr(TOKEN_COND);
			*symbol_tmp = (int)(g_byteCode_it+1);
		}
		//logical or ||
		else if(g_currentToken == TOKEN_LOR)
		{
			nextToken();
			*++g_byteCode_it = INS_BNZ;
			symbol_tmp = ++g_byteCode_it;
			expr(TOKEN_LAND);
			*symbol_tmp = (int)(g_byteCode_it+1);
			g_currentType = TYPE_INT;
		}
		//logical and &&
		else if(g_currentToken == TOKEN_LAND)
		{
			nextToken();
			*++g_byteCode_it = INS_BZ;
			symbol_tmp = ++g_byteCode_it;
			expr(TOKEN_OR);
			*symbol_tmp = (int)(g_byteCode_it + 1);
			g_currentType = TYPE_INT;
		}
		//or |
		else if(g_currentToken == TOKEN_OR)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_XOR);
			*++g_byteCode_it = INS_OR;
			g_currentType = TYPE_INT;
		}
		//xor ^
		else if(g_currentToken == TOKEN_XOR)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_AND);
			*++g_byteCode_it = INS_XOR;
			g_currentType = TYPE_INT;
		}
		//and &
		else if(g_currentToken == TOKEN_AND)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_EQ);
			*++g_byteCode_it = INS_AND;
			g_currentType = TYPE_INT;
		}
		//equal ==
		else if(g_currentToken == TOKEN_EQ)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_LE);
			*++g_byteCode_it = INS_EQ;
			g_currentType = TYPE_INT;
		}
		//not equal !=
		else if(g_currentToken == TOKEN_NEQ)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_LE);
			*++g_byteCode_it = INS_NEQ;
			g_currentType = TYPE_INT;
		}
		//less than <
		else if(g_currentToken == TOKEN_LE)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_SHL);
			*++g_byteCode_it = INS_LE;
			g_currentType = TYPE_INT;
		}
		//greater than >
		else if(g_currentToken == TOKEN_GR)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_SHL);
			*++g_byteCode_it = INS_GR;
			g_currentType = TYPE_INT;
		}
		//less than or equal <=
		else if(g_currentToken == TOKEN_LEQ)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_SHL);
			*++g_byteCode_it = INS_LEQ;
			g_currentType = TYPE_INT;
		}
		//greater than or equal >=
		else if(g_currentToken == TOKEN_GEQ)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_SHL);
			*++g_byteCode_it = INS_GEQ;
			g_currentType = TYPE_INT;
		}
		//shift left <<
		else if(g_currentToken == TOKEN_SHL)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_ADD);
			*++g_byteCode_it = INS_SHL;
			g_currentType = TYPE_INT;
		}
		//shift right >>
		else if(g_currentToken == TOKEN_SHR)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_ADD);
			*++g_byteCode_it = INS_SHR;
			g_currentType = TYPE_INT;
		}
		//add token +
		else if(g_currentToken == TOKEN_ADD)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_MUL);
			if((g_currentType = counter) > TYPE_PTR)
			{
				*++g_byteCode_it = INS_PSH;
				*++g_byteCode_it = INS_IMM;
				*++g_byteCode_it = sizeof(int);
				*++g_byteCode_it = INS_MUL;
			}
			*++g_byteCode_it = INS_ADD;
		}
		//sub token -
		else if(g_currentToken == TOKEN_SUB)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_MUL);
			if((g_currentType = counter) > TYPE_PTR)
			{
				*++g_byteCode_it = INS_PSH;
				*++g_byteCode_it = INS_IMM;
				*++g_byteCode_it = sizeof(int);
				*++g_byteCode_it = INS_MUL;
			}
			*++g_byteCode_it = INS_SUB;
		}
		//mul *
		else if(g_currentToken == TOKEN_MUL)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_INC);
			*++g_byteCode_it = INS_MUL;
			g_currentType = TYPE_INT;
		}
		//div /
		else if(g_currentToken == TOKEN_DIV)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_INC);
			*++g_byteCode_it = INS_DIV;
			g_currentType = TYPE_INT;
		}
		//mod %
		else if(g_currentToken == TOKEN_MOD)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_INC);
			*++g_byteCode_it = INS_MOD;
			g_currentType = TYPE_INT;
		}
		//inc || Dec ++ --
		else if(g_currentToken == TOKEN_INC || g_currentToken == TOKEN_DEC)
		{
			if(*g_byteCode_it == INS_LOADCHAR)
			{
				*g_byteCode_it = INS_PSH;
				*++g_byteCode_it = INS_LOADCHAR;
			}
			else if(*g_byteCode_it == INS_LOADINT)
			{
				*g_byteCode_it = INS_PSH;
				*++g_byteCode_it = INS_LOADINT;
			}
			else
			{
				printf("%d: bad lvalue in post-increment", g_currentLine);
				exit(-1);
			}
			*++g_byteCode_it = INS_PSH;
			*++g_byteCode_it = INS_IMM;
			*++g_byteCode_it = (g_currentType > TYPE_PTR) ? sizeof(int) : sizeof(char);
			*++g_byteCode_it = (g_currentToken == TOKEN_INC) ? INS_ADD : INS_SUB;
			*++g_byteCode_it = (g_currentType == TYPE_CHAR) ? INS_SAVECHAR : INS_SAVEINT;
			*++g_byteCode_it = INS_PSH;
			*++g_byteCode_it = INS_IMM;
			*++g_byteCode_it = (g_currentType > TYPE_PTR) ? sizeof(int) : sizeof(char);
			*++g_byteCode_it = (g_currentToken == TOKEN_INC) ? INS_SUB : INS_ADD;
			nextToken();
		}
		//bracket ]
		else if(g_currentToken == TOKEN_BRACKET)
		{
			nextToken();
			*++g_byteCode_it = INS_PSH;
			expr(TOKEN_ASSIGN);
			if(g_currentToken == ']')
				nextToken();
			else
			{
				printf("%d: close bracket expected\n", g_currentLine);
				exit(-1);
			}
			if(counter > TYPE_PTR)
			{
				*++g_byteCode_it = INS_PSH;
				*++g_byteCode_it = INS_IMM;
				*++g_byteCode_it = sizeof(int);
				*++g_byteCode_it = INS_MUL;
			}
			else if(counter < TYPE_PTR)
			{
				printf("%d: pointer type expected\n", g_currentLine);
				exit(-1);
			}

			*++g_byteCode_it = INS_ADD;
			*++g_byteCode_it = ((g_currentType = counter - TYPE_PTR) == TYPE_CHAR) ? INS_LOADCHAR : INS_LOADINT;
		}
		else
		{
			printf("%d: compiler error\n", g_currentLine);
			printCurrentToken();
			exit(-1);
		}
	}
	return;
}

//parses a statement
//@stag stmt
void
stmt()
{
	int *tmp_a, *tmp_b;

	//if stmt
	if(g_currentToken == TOKEN_IF)
	{
		nextToken();
		if(g_currentToken == '(')
			nextToken();
		else
		{
			printf("%d: open paren expected\n", g_currentLine);
			exit(-1);
		}

		//condition expr
		expr(TOKEN_ASSIGN);

		if(g_currentToken == ')')
			nextToken();
		else
		{
			printf("%d: close paren expected in if statement\n", g_currentLine);
			exit(-1);
		}

		//branch if zero to the end of the if body
		*++g_byteCode_it = INS_BZ;
		//save the position to offset until we calc it
		tmp_b = ++g_byteCode_it;

		//parse if body
		stmt();

		if(g_currentToken == TOKEN_ELSE)
		{
			*tmp_b = (int)(g_byteCode_it+3);
			//same fam
			*++g_byteCode_it = INS_JMP;
			tmp_b = ++g_byteCode_it;
			nextToken();
			//parse else body
			stmt();
		}

		*tmp_b = (int)(g_byteCode_it+1);
	}
	//while stmt
	else if(g_currentToken == TOKEN_WHILE)
	{
		nextToken();
		//start of while to jump to
		tmp_a = g_byteCode_it+1;

		if(g_currentToken == '(')
			nextToken();
		else
		{
			printf("%d: open paren expected\n", g_currentLine);
			exit(-1);
		}

		//expr
		expr(TOKEN_ASSIGN);

		if(g_currentToken == ')')
			nextToken();
		else
		{
			printf("%d: close paren expected in while statement\n", g_currentLine);
			exit(-1);
		}

		//this exits the loop if the value of register is zero
		*++g_byteCode_it = INS_BZ;
		tmp_b = ++g_byteCode_it;

		stmt();

		//go up to the start of the loop
		*++g_byteCode_it = INS_JMP;
		*++g_byteCode_it = (int)tmp_a;

		//set the offset to break out of the loop
		*tmp_b = (int)(g_byteCode_it+1);
	}
	//return stmt
	else if(g_currentToken == TOKEN_RETURN)
	{
		nextToken();
		//expr
		if(g_currentToken != ';')
		{
			expr(TOKEN_ASSIGN);
		}

		//leave the scope
		*++g_byteCode_it = INS_LEAVE;

		if(g_currentToken == ';')
			nextToken();
		else
		{
			printf("%d: semicolon expected\n", g_currentLine);
			exit(-1);
		}
	}
	//block stmt
	else if(g_currentToken == '{')
	{
		nextToken();
		while(g_validToken && g_currentToken != '}')
			stmt();
		nextToken();
	}
	//semicolon stmt
	else if(g_currentToken == ';')
	{
		nextToken();
	}
	//expr stmt
	else
	{
		expr(TOKEN_ASSIGN);
		if(g_currentToken == ';')
			nextToken();
		else
		{
			printf("%d: semicolon expected\n", g_currentLine);
			exit(-1);
		}
	}

}


//@stag debug_exit
// void
// debug_exit(void)
// {
// 	printf("EXITING...\n");
// 	printCurrentToken();
// 	printSymbolTable();
// 	printByteCode();
// }


// Compiler steps 

// - parse process arguments
// - read the files being fed to the compiler
// - insert the keywords into the symbol table
// - insert the system functions into the symbol table
// - scan -> parse -> generate byte code to the code
// - prepare the stack
// - fetch -> decode -> execute the byte code


//@stag main
int main(int argc, char** argv)
{
	int file_handle,		//file handle to the current file compiling
		pool_size,			//pool size that controls the size of memory blocks
		base_type,			//base type for trivial semantic analysis and shit
		current_type, 		//type of the current thing being compiled
		debug_flag, 		//indicates the debug flag to print the instructions as executed
		*main_entry;		//keeps track of the main funciton entry point

	//temps
	int i, tmp_token, *symbol_it, *int_ptr;

	//virtual machine registers
	int *program_counter, *stack_pointer, *base_pointer, a, cycle_counter;

	char* char_ptr;

	// atexit(debug_exit);


	//configurations
	pool_size = 256 * 1024; //256 KB

	debug_flag = 0;

	//skip the process name
	--argc; ++argv;

	//check for debug flag
	if(argc > 0 && **argv == '-' && (*argv)[1] == 'd')
	{
		debug_flag = 1;
		--argc;
		++argv;
	}
	//if no arguments were passed then print the help message and exit
	if(argc < 1)
	{
		printf("usage: C4 file ...\n");
		return -1;
	}

	//start loading the files
	if((file_handle = open(*argv, 0)) < 0)
	{
		printf("could not open(%s)\n", *argv);
		return -1;
	}

	if(!(g_data_stream = g_stream_it = malloc(pool_size)))
	{
		printf("could not malloc(%d) source stream buffer\n", pool_size);
		return -1;
	}

	if((i = read(file_handle, g_data_stream, pool_size-1)) <= 0)
	{
		printf("read() failed and returned %d\n", i);
		return -1;
	}

	//symbol table initialization
	if(!(g_symbolTable = malloc(pool_size)))
	{
		printf("could not malloc(%d) symbol area", pool_size);
		return -1;
	}
	memset(g_symbolTable, 0, pool_size);

	//data area allocation
	if(!(g_data = malloc(pool_size)))
	{
		printf("could not malloc(%d) data area", pool_size);
		return -1;
	}
	g_data_it = g_data;
	memset(g_data, 0, pool_size);

	//stack area allocation
	if(!(stack_pointer = malloc(pool_size)))
	{
		printf("could not malloc(%d) stack area", pool_size);
		return -1;
	}

	//byte code allocation
	if(!(g_byteCode = malloc(pool_size)))
	{
		printf("could not malloc(%d) byte code area", pool_size);
		return -1;
	}
	memset(g_byteCode, 0, pool_size);
	g_byteCode_it = g_byteCode;

	// set the keywords to symbol table
	g_stream_it = "char else enum if int return sizeof while "
				"open read close printf malloc memset memcmp memcpy mmap dlsym qsort exit void main";
	tmp_token = TOKEN_CHAR;
	nextToken();
	while(g_validToken && tmp_token <= TOKEN_WHILE)
	{
		g_currentID[SYM_TOKEN] = tmp_token++;
		nextToken();
	}

	//set the keywords to the libc functions
	tmp_token = INS_OPEN;
	while(g_validToken && tmp_token <= INS_EXIT)
	{
		g_currentID[SYM_VALUE] = tmp_token++;
		g_currentID[SYM_CLASS] = CLASS_SYSTEM;
		g_currentID[SYM_TYPE]  = TYPE_INT;
		nextToken();
	}

	//add void
	g_currentID[SYM_TOKEN] = TYPE_CHAR; //treat void as char
	nextToken();
	main_entry = g_currentID; //hold the main id as entry


	//put the iterator at the begining of the stream
	g_stream_it = g_data_stream;

	//set the current line being compiled to the start of the file
	g_currentLine = 1;

	//actually start lexing the file
	nextToken();

	//by default the globals are zeros so the comparison holds
	while(g_validToken)
	{
		//c doesn't have statements it has declarations within those declarations there exist statments
		//assume that the base type of every thing is an int
		base_type = TYPE_INT;

		//parsing the type

		//if it happens that the first token is an int
		if(g_currentToken == TOKEN_INT)
		{
			nextToken();
		}
		//if it's char then change the base type
		else if(g_currentToken == TOKEN_CHAR)
		{
			base_type = TYPE_CHAR;
			nextToken();
		}
		//if it's an enum then change base type
		else if(g_currentToken == TOKEN_ENUM)
		{
			nextToken();
			//we don't care about the enum name so if it exists skip it
			if(g_currentToken != '{')
				nextToken();
			//parse the enum
			if(g_currentToken == '{')
			{
				nextToken();
				i = 0;
				//while we are within the enum
				while(g_currentToken != '}')
				{
					if(g_currentToken != TOKEN_ID)
					{
						printf("%d: bad enum identifier %s\n", g_currentLine, tokenName(g_currentToken));
						return -1;
					}


					//get the next token which might be an =
					nextToken();
					if(g_currentToken == TOKEN_ASSIGN)
					{
						//get the next number as value for this enum entry
						nextToken();
						if(g_currentToken != TOKEN_NUM)
						{
							printf("%d: bad enum initializer %s\n", g_currentLine, tokenName(g_currentToken));
							return -1;
						}
						i = g_tokenValue;
						nextToken();
					}
					//set the symbol table entry to be a number
					g_currentID[SYM_CLASS] = CLASS_NUM;
					//set it's type to be an int
					g_currentID[SYM_TYPE] = TYPE_INT;
					//set it's value to be whatever inside i
					g_currentID[SYM_VALUE] = i++;

					if(g_currentToken == ',')
						nextToken();
				}
				nextToken();
			}
		}

		//parsing the expression
		while(g_currentToken != ';' && g_currentToken != '}')
		{
			//printCurrentToken();
			//set the current type to be the base types
			current_type = base_type;
			while(g_currentToken == TOKEN_MUL)
			{
				nextToken();
				current_type = current_type + TYPE_PTR;
			}

			//so after putting a type and any amount of pointers you want now it's time for an id
			if(g_currentToken != TOKEN_ID)
			{
				printf("%d: bad global declaration %s\n", g_currentLine, tokenName(g_currentToken));
				return -1;
			}

			//and this id must have no class since we are going to assign class to it next
			//if it had a class already then it must be a redeclaration of some sort
			if(g_currentID[SYM_CLASS] != 0)
			{
				//printf("1\n");
				printf("%d: redeclaration of variable %.*s\n", g_currentLine, g_currentID[SYM_NSIZE], g_currentID[SYM_NAME]);
				return -1;
			}

			//assign the type of this symbol table entry
			g_currentID[SYM_TYPE] = current_type;

			//get the next token
			nextToken();

			//if it's a function
			if(g_currentToken == '(')
			{
				g_currentID[SYM_CLASS] = CLASS_FUNNCTION;
				g_currentID[SYM_VALUE] = (int)(g_byteCode_it);
				//printf("func address: %d\n", g_currentID[SYM_VALUE]);
				//parse the function arguments
				nextToken();
				i=0;
				while(g_currentToken != ')')
				{
					//same routine get the type then expect an id
					current_type = TYPE_INT;
					if(g_currentToken == TOKEN_INT)
						nextToken();
					else if(g_currentToken == TOKEN_CHAR)
					{
						nextToken();
						current_type = TYPE_CHAR;
					}

					while(g_currentToken == TOKEN_MUL)
					{
						nextToken();
						current_type = current_type + TYPE_PTR;
					}

					if(g_currentToken != TOKEN_ID)
					{
						printf("%d: bad parameter declaration %s\n", g_currentLine, tokenName(g_currentToken));
						return -1;
					}

					//if variable defined in local scope then it's a redeclaration
					if(g_currentID[SYM_CLASS] == CLASS_LOCAL)
					{
						//printf("2\n");
						printf("%d: redeclaration of variable %.*s\n", g_currentLine, g_currentID[SYM_NSIZE], g_currentID[SYM_NAME]);
						return -1;
					}

					//save the previous in case of shadowing else we are good still
					g_currentID[SYM_TCLASS] = g_currentID[SYM_CLASS];
					g_currentID[SYM_CLASS] = CLASS_LOCAL;

					g_currentID[SYM_TTYPE] = g_currentID[SYM_TYPE];
					g_currentID[SYM_TYPE] = current_type;

					g_currentID[SYM_TVALUE] = g_currentID[SYM_VALUE];
					g_currentID[SYM_VALUE] = i++;

					nextToken();
					if(g_currentToken == ',')
						nextToken();
				} //end of arguments parsing

				//parse function definition
				nextToken();
				if(g_currentToken != '{')
				{
					printf("%d: bad function definition\n", g_currentLine);
					return -1;
				}

				g_localScope = ++i;

				nextToken();
				//parse the function variable declaration
				while(g_currentToken == TOKEN_INT || g_currentToken == TOKEN_CHAR)
				{
					base_type = (g_currentToken == TOKEN_INT) ? TYPE_INT : TYPE_CHAR;
					nextToken();

					while(g_currentToken != ';')
					{
						current_type = base_type;
						while(g_currentToken == TOKEN_MUL)
						{
							nextToken();
							current_type = current_type + TYPE_PTR;
						}

						if(g_currentToken != TOKEN_ID)
						{
							printf("%d: bad parameter declaration %s\n", g_currentLine, tokenName(g_currentToken));
							return -1;
						}

						if(g_currentID[SYM_CLASS] == CLASS_LOCAL)
						{
							//("3\n");
							printf("%d: redeclaration of variable %.*s\n", g_currentLine, g_currentID[SYM_NSIZE], g_currentID[SYM_NAME]);
							return -1;
						}

						g_currentID[SYM_TCLASS] = g_currentID[SYM_CLASS];
						g_currentID[SYM_CLASS] = CLASS_LOCAL;

						g_currentID[SYM_TTYPE] = g_currentID[SYM_TYPE];
						g_currentID[SYM_TYPE] = current_type;

						g_currentID[SYM_TVALUE] = g_currentID[SYM_VALUE];
						g_currentID[SYM_VALUE] = ++i;

						nextToken();
						if(g_currentToken == ',')
							nextToken();
					}
					nextToken();
				} //end of function variable declaration

				*g_byteCode_it = INS_NOINS;
				*++g_byteCode_it = INS_ENTER;

				*++g_byteCode_it = i - g_localScope; //enters the scope with the parameters of the function

				//parses the statment of the function until the end
				while(g_currentToken != '}')
					stmt();

				*++g_byteCode_it = INS_LEAVE;

				//revert the shadowing
				symbol_it = g_symbolTable;

				while(symbol_it[SYM_TOKEN])
				{
					if(symbol_it[SYM_CLASS] == CLASS_LOCAL)
					{
						symbol_it[SYM_CLASS] = symbol_it[SYM_TCLASS];
						symbol_it[SYM_TYPE] = symbol_it[SYM_TTYPE];
						symbol_it[SYM_VALUE] = symbol_it[SYM_TVALUE];
					}
					symbol_it = symbol_it + SYM_SIZE;
				}

			} // end of function parsing
			//if it's not a function then it's a variable
			else
			{
				g_currentID[SYM_CLASS] = CLASS_GLOBAL;
				g_currentID[SYM_VALUE] = (int)g_data_it;
				g_data_it = g_data_it + sizeof(int);
			}

			if(g_currentToken == ',')
				nextToken();

		} //end of declaration parsing
		nextToken();
	} // end of global stream of valid tokens

	if(!(program_counter = (int* )main_entry[SYM_VALUE]))
	{
		printf("entry point main not defined\n");
		return -1;
	}

	//set the stack_pointer to the end to grow upward
	stack_pointer = (int *)((int)stack_pointer + pool_size);
	*--stack_pointer = INS_EXIT; //terminate the process
	*--stack_pointer = INS_PSH; //move the stack to accomodate the args to the main(argc, argv)
	int_ptr = stack_pointer;
	*--stack_pointer = argc;
	*--stack_pointer = (int)argv;
	*--stack_pointer = (int)int_ptr;

	//debug_exit();
	//execution loop
	cycle_counter = 0;
	//fetch decode execute
	while(1)
	{
		//fetch
		i = *program_counter++;
		++cycle_counter;

		//print the instruction being executed
		if(debug_flag != 0)
		{
			printf("%d> ", cycle_counter);
			if(i == INS_LEA)
			{
				printf("LEA %d\n", *program_counter);
			}
			else if(i == INS_IMM)
			{
				printf("IMM %d\n", *program_counter);
			}
			else if(i == INS_JMP)
			{
				printf("JMP %d\n", *program_counter);
			}
			else if(i == INS_JSR)
			{
				printf("JSR %d\n", *program_counter);
			}
			else if(i == INS_BZ)
			{
				printf("BZ  %d\n", *program_counter);
			}
			else if(i == INS_BNZ)
			{
				printf("BNZ %d\n", *program_counter);
			}
			else if(i == INS_ENTER)
			{
				printf("ENTER %d\n", *program_counter);
			}
			else if(i == INS_ADJ)
			{
				printf("ADJ %d\n", *program_counter);
			}
			else
			{
				char_ptr = insName(i);
				if(char_ptr)
				{
					printf("%s\n", char_ptr);
				}
				else
				{
					printf("unidentified instruction\n");
				}
			}
		}

		//decode execute
		if(i == INS_LEA) 		//load local address
		{
			//add the base pointer to the offset of the variable
			a = (int)(base_pointer + *program_counter++);
		}
		else if(i == INS_IMM) 	//load immediate value
		{
			a = *program_counter++;
		}
		else if (i == INS_JMP) 	//jumps to the next position in code
		{
			program_counter = (int *)*program_counter;
		}
		else if(i == INS_JSR) 	//jumps to a function
		{
			*--stack_pointer = (int)(program_counter+1);	//save the current position
			program_counter = (int*) *program_counter;	//jump
		}
		else if(i == INS_BZ)	//branches if zero
		{
			program_counter = a ? program_counter + 1 : (int *)*program_counter;
		}
		else if(i == INS_BNZ) 	//branches if not zero
		{
			program_counter = a ? (int *)*program_counter : program_counter + 1;
		}
		else if(i == INS_ENTER)	//enters a function
		{
			*--stack_pointer = (int) base_pointer;	//save the previous scope
			base_pointer = stack_pointer; 			//this is the start of the scope
			stack_pointer = stack_pointer - *program_counter++;	//allocate local variables
		}
		else if(i == INS_ADJ)	//adjusts stack size
		{
			stack_pointer = stack_pointer + *program_counter++;
		}
		else if(i == INS_LEAVE) //leaves a function
		{
			stack_pointer = base_pointer; 	//deallocate the local variables
			base_pointer = (int*)*stack_pointer++; 	//restore the previous scope
			program_counter = (int *)*stack_pointer++; //restore the program counter
		}
		else if(i == INS_LOADINT)
		{
			a = *(int *)a; 		//loads an int which a register holds its address
		}
		else if(i == INS_LOADCHAR)
		{
			a = *(char *)a; 	//loads a char which a register holds its address
		}
		else if(i == INS_SAVEINT)
		{
			*(int*)*stack_pointer++ = a; 	//saves an int in the stack
		}
		else if(i == INS_SAVECHAR)
		{
			a = *(char *)*stack_pointer++ = a; 	//saves a char in the stack then get the value of it as char in a register
		}
		else if(i == INS_PSH)	//pushes a value in a register into stack
		{
			*--stack_pointer = a;
		}
		else if(i == INS_OR)
		{
			a = *stack_pointer++ | a;
		}
		else if(i == INS_XOR)
		{
			a = *stack_pointer++ ^ a;
		}
		else if(i == INS_AND)
		{
			a = *stack_pointer++ & a;
		}
		else if(i == INS_EQ)
		{
			a = *stack_pointer++ == a;
		}
		else if(i == INS_NEQ)
		{
			a = *stack_pointer++ != a;
		}
		else if(i == INS_LE)
		{
			a = *stack_pointer++ < a;
		}
		else if(i == INS_GR)
		{
			a = *stack_pointer++ > a;
		}
		else if(i == INS_LEQ)
		{
			a = *stack_pointer++ <= a;
		}
		else if(i == INS_GEQ)
		{
			a = *stack_pointer++ >= a;
		}
		else if(i == INS_SHL)
		{
			a = *stack_pointer++ << a;
		}
		else if(i == INS_SHR)
		{
			a = *stack_pointer++ >> a;
		}
		else if(i == INS_ADD)
		{
			a = *stack_pointer++ + a;
		}
		else if(i == INS_SUB)
		{
			a = *stack_pointer++ - a;
		}
		else if(i == INS_MUL)
		{
			a = *stack_pointer++ * a;
		}
		else if(i == INS_DIV)
		{
			a = *stack_pointer++ / a;
		}
		else if(i == INS_MOD)
		{
			a = *stack_pointer++ % a;
		}
		else if(i == INS_OPEN)
		{
			a = open((char*)stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_READ)
		{
			a = read(stack_pointer[2], (char*)stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_CLOSE)
		{
			a = close(*stack_pointer);
		}
		else if(i == INS_PRINTF)
		{
			int_ptr = stack_pointer + program_counter[1];
			a = printf((char*)int_ptr[-1], int_ptr[-2], int_ptr[-3], int_ptr[-4], int_ptr[-5], int_ptr[-6]);
		}
		else if(i == INS_MALLOC)
		{
			a = (int)malloc(*stack_pointer);
		}
		else if(i == INS_MEMSET)
		{
			a = (int)memset((char *)stack_pointer[2], stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_MEMCMP)
		{
			a = memcmp((char*)stack_pointer[2], (char*)stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_MEMCPY)
		{
			a = (int)memcpy((char*)stack_pointer[2], stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_MMAP)
		{
			a = (int)mmap((char*)stack_pointer[5], stack_pointer[4], stack_pointer[3], stack_pointer[2], stack_pointer[1], *stack_pointer);
		}
		else if(i == INS_DLSYM)
		{
			a = (int)dlsym((char*)stack_pointer[1], (char*)*stack_pointer);
		}
		else if(i == INS_QSORT)
		{
			qsort((char*)stack_pointer[3], stack_pointer[2], stack_pointer[1], (void*)*stack_pointer);
		}
		else if(i == INS_EXIT)	//terminates the process
		{
			printf("exit code: (%d), cycle counter: (%d)\n", *stack_pointer, cycle_counter);
			return *stack_pointer;
		}else if(i == INS_NOINS)
		{
			//do nothing this is a no instruction
		}
		else
		{
			printf("unidentified instruction = %d!! cycle counter = %d\n", i, cycle_counter);
			return -1;
		}
	}
	//printSymbolTable();
	//printByteCode();
	return 0;
}