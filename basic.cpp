#include "stdafx.h"
#include "basic.h"
#include <stdarg.h>


extern "C"
{
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include "linkedlist.h"
#include "expr.h"
}

void main()
{
	basic_main();
}

#define _BUILD_NUM_ "0.1.0"

void basic_main()
{
	printf("\n                 The Raspberry Pi BASIC Development System");
	printf("\n\n                       Operating System Version %s", _BUILD_NUM_);
	printf("\n\nReady.\n");

	struct Context ctx;
	unsigned char linebuf[LINE_SZ];
	int linenum, x;
	int dataPtr = 0;
	unsigned char cmd[CMD_NAMESZ];

	clear((unsigned char *)&ctx, sizeof(ctx));
	exec_init(&ctx);

	while (1)
	{
		get_input(linebuf);
		to_uppercase(linebuf);
		/* process line of code */
		if (ISDIGIT(linebuf[0]))
		{
			// get the full line number
			dataPtr = get_int(linebuf, 0, &linenum);
			while(linebuf[dataPtr] == ' ') dataPtr++;
			memcpy(linebuf, linebuf + dataPtr, length(linebuf) + dataPtr);

			// did user enter empty line number?
			if (isemptyline(linebuf))
				ll_delete(linenum);
			else
			{
				struct node* nodeLine = ll_find(linenum);
				unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * length(linebuf)+1);
				memcpy(data, linebuf, length(linebuf)+1);

				// If the line doesnt exist, just add it.
				// otherwise free the previous data memory and add the new line
				if (nodeLine == NULL)
					ll_insertFirst(linenum, data);
				else
				{
					free(nodeLine->data);
					nodeLine->data = data;
				}	
			}

			continue;
		}
		else
		{
			unsigned char* tokenized_line = (unsigned char*)malloc(sizeof(unsigned char) * 160);
			tokenize(&ctx, linebuf, tokenized_line);
			ctx.tokenized_line = tokenized_line;
			ctx.linePos = 0;

			exec_line(&ctx);
			ctx.error_line = -1;
			handle_error(&ctx);

			free(tokenized_line);

			printf("\nReady.\n");
		}
	}
}

void exec_init(struct Context *ctx)
{
	BINDCMD(&ctx->cmds[0], "PRINT", true, exec_cmd_print, TOKEN_PRINT);
	BINDCMD(&ctx->cmds[1], "INPUT", false, exec_cmd_input, TOKEN_INPUT);
	BINDCMD(&ctx->cmds[2], "RETURN", true, exec_cmd_return, TOKEN_RETURN);
	BINDCMD(&ctx->cmds[3], "GOTO", true, exec_cmd_goto, TOKEN_GOTO);
	BINDCMD(&ctx->cmds[4], "GOSUB", true, exec_cmd_gosub, TOKEN_GOSUB);
	BINDCMD(&ctx->cmds[5], "LET", true, exec_cmd_let, TOKEN_LET);
	BINDCMD(&ctx->cmds[6], "END", true, exec_cmd_end, TOKEN_END);
	BINDCMD(&ctx->cmds[7], "STOP", true, exec_cmd_stop, TOKEN_STOP);
	BINDCMD(&ctx->cmds[8], "IF", true, exec_cmd_if, TOKEN_IF);
	BINDCMD(&ctx->cmds[9], "DIM", true, exec_cmd_dim, TOKEN_DIM);
	BINDCMD(&ctx->cmds[10], "REM", true, exec_cmd_rem, TOKEN_REM);
	BINDCMD(&ctx->cmds[11], "THEN", true, NULL, TOKEN_THEN);
	BINDCMD(&ctx->cmds[12], "AND", true, NULL, TOKEN_AND);
	BINDCMD(&ctx->cmds[13], "OR", true, NULL, TOKEN_OR);
	BINDCMD(&ctx->cmds[14], "ABS", true, NULL, TOKEN_ABS);
	BINDCMD(&ctx->cmds[15], "FOR", true, exec_cmd_for, TOKEN_FOR);
	BINDCMD(&ctx->cmds[16], "TO", true, NULL, TOKEN_TO);
	BINDCMD(&ctx->cmds[17], "NEXT", true, exec_cmd_next, TOKEN_NEXT);
	BINDCMD(&ctx->cmds[18], "STEP", true, NULL, TOKEN_STEP);
	BINDCMD(&ctx->cmds[19], "NEW", true, exec_cmd_new, TOKEN_NEW);
	BINDCMD(&ctx->cmds[20], "LOAD", true, exec_cmd_load, TOKEN_LOAD);
	BINDCMD(&ctx->cmds[21], "SAVE", true, exec_cmd_save, TOKEN_SAVE);
	BINDCMD(&ctx->cmds[22], "LIST", true, exec_cmd_list, TOKEN_LIST);
	BINDCMD(&ctx->cmds[23], "RUN", true, exec_cmd_run, TOKEN_RUN);
	BINDCMD(&ctx->cmds[24], "CHR$", true, NULL, TOKEN_CHR$);
	BINDCMD(&ctx->cmds[25], "LEN", true, NULL, TOKEN_LEN);
	BINDCMD(&ctx->cmds[26], "ASC", true, NULL, TOKEN_ASC);
	BINDCMD(&ctx->cmds[27], "VAL", true, NULL, TOKEN_VAL);
	BINDCMD(&ctx->cmds[28], "STR$", true, NULL, TOKEN_STR$);
	BINDCMD(&ctx->cmds[29], "SGN", true, NULL, TOKEN_SGN);
	BINDCMD(&ctx->cmds[30], "INT", true, NULL, TOKEN_INT);
	BINDCMD(&ctx->cmds[31], "SQR", true, NULL, TOKEN_SQR);
	BINDCMD(&ctx->cmds[32], "RND", true, NULL, TOKEN_RND);
	BINDCMD(&ctx->cmds[33], "LOG", true, NULL, TOKEN_LOG);
	BINDCMD(&ctx->cmds[34], "EXP", true, NULL, TOKEN_EXP);
	BINDCMD(&ctx->cmds[35], "COS", true, NULL, TOKEN_COS);
	BINDCMD(&ctx->cmds[36], "SIN", true, NULL, TOKEN_SIN);
	BINDCMD(&ctx->cmds[37], "TAN", true, NULL, TOKEN_TAN);
	BINDCMD(&ctx->cmds[38], "ATN", true, NULL, TOKEN_ATN);
	BINDCMD(&ctx->cmds[39], "LEFT$", true, NULL, TOKEN_LEFT$);
	BINDCMD(&ctx->cmds[40], "CONT", true, exec_cmd_cont, TOKEN_CONT);
	BINDCMD(&ctx->cmds[41], "RIGHT$", true, NULL, TOKEN_RIGHT$);
	BINDCMD(&ctx->cmds[42], "MID$", true, NULL, TOKEN_MID$);
}

void exec_program(struct Context* ctx)
{
	int ci;

	ll_sort();

	struct node *currentNode = ll_gethead();

	ctx->running = true;
	ctx->jmpline = -1;
	ctx->line = 0;
	ctx->dsptr = 0;
	ctx->csptr = 0;
	ctx->allocated = 0;
	ctx->linePos = 0;
	ctx->error_line = 0;
	ctx->error = ERR_NONE;

	forstackidx = 0;

	var_clear_all(ctx);

	while (ctx->running && currentNode != NULL)
	{
		ctx->original_line = currentNode->data;

		unsigned char* tokenized_line = (unsigned char*)malloc(sizeof(unsigned char) * 160);
		tokenize(ctx, ctx->original_line, tokenized_line);

		ctx->tokenized_line = tokenized_line;
		ctx->line = currentNode->linenum;

		exec_line(ctx);

		free(ctx->tokenized_line);

		/*char ch = getchar();

		if (ch == 27)
		{
			ctx->running = false;
			ctx->error = ERR_UNEXP;
			printf("\n?Break in %d\n", ctx->line);
			return;
		}*/
		
		handle_error(ctx);
		if (!ctx->running) return;

		if (ctx->jmpline == -1)
		{
			currentNode = currentNode->next;
			ctx->linePos = 0;
		}	
		else
		{
			currentNode = ll_find(ctx->jmpline);
			ctx->jmpline = -1;
		}
	}
}

void exec_line(struct Context *ctx)
{
	int origLinePos;
	int j;
	unsigned char cmd[CMD_NAMESZ];
	bool foundCmd = false;

	while(true)
	{
		foundCmd = false;
		ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

		if (ctx->linePos == -1)
			break;

		origLinePos = ctx->linePos;
		ctx->linePos = get_symbol(ctx->tokenized_line, ctx->linePos, cmd);

		for (j = 0; j < CMD_COUNT; j++)
		{
			if (cmd[0] == ctx->cmds[j].token && ctx->cmds[j].func != NULL)
			{
				foundCmd = true;
				// execute the statement
				ctx->cmds[j].func(ctx);
				break;
			}
		}

		// Assume LET for anything else
		if (!foundCmd)
		{
			ctx->linePos = origLinePos;
			exec_cmd_let(ctx);
		}

		if (ctx->error != ERR_NONE)
			return;

		if (ctx->jmpline != -1)
			break;

		if (ctx->linePos != -1)
		{
			if (ctx->tokenized_line[ctx->linePos] == ':')
				ctx->linePos++;
		}
	}
}

void handle_error(struct Context *ctx)
{
	if (ctx->error != ERR_NONE)
	{
		switch (ctx->error)
		{
		case ERR_SYNTAX:
			printf("\n?Syntax error");
			break;
		case ERR_UNDEFSTATEMENT:
			printf("\n?Undef'd statement error");
			break;
		case ERR_BREAK:
			printf("\n?Break");
			break;
		case ERR_DIV0:
			printf("\n?Division by zero error");
			break;
		case ERR_RTRN_WO_GSB:
			printf("\n?RETURN without GOSUB error");
			break;
		case ERR_TYPE_MISMATCH:
			printf("\n?Type mismatch error");
			break;
		case ERR_NEXT_WO_FOR:
			printf("\n?NEXT without FOR error");
			break;
		case ERR_ILLEGAL_DIRECT:
			printf("\n?Illegal direct error");
			break;
		case ERR_ILLEGAL_QTY:
			printf("\n?Illegal quantity error");
			break;
		default:
			printf("\n?Unspecified error");
			break;
		}

		if (ctx->error_line != -1)
			printf(" in %d", ctx->error_line);

		ctx->error = ERR_NONE;
		ctx->running = false;
		return;
	}
}

int exec_expr(struct Context *ctx)
{
	unsigned char exp[200];
	int ctr = 0;
	int lpos = ctx->linePos;
	double result = 0;
	bool funcMode = false;

	while (true)
	{
		//stop at line terminators or delimiters
		if (lpos == -1 || ctx->tokenized_line[lpos] == 0 || ctx->tokenized_line[lpos] == ':' || ctx->tokenized_line[lpos] == ';')
			break;

		if (ctx->tokenized_line[lpos] == '(')
			funcMode = true;

		if (ctx->tokenized_line[lpos] == ',' && funcMode == false)
			break;

		if (ctx->tokenized_line[lpos] == ')')
			funcMode = false;

		// stop at keywords (but not funtions)
		if (ctx->tokenized_line[lpos]  > 128 && (ctx->tokenized_line[lpos] < 174 || ctx->tokenized_line[lpos] > 202))
			break;

		if (ctx->expr_boundary != 0 && lpos > ctx->expr_boundary)
			break;

		exp[ctr++] = ctx->tokenized_line[lpos++];
	}

	exp[ctr] = 0;

	unsigned char* strResult = (unsigned char*)malloc(sizeof(unsigned char) * 255);
	
	ctx->error = expr_eval(exp, strResult, ctx);
	ctx->expression_result = strResult;
	ctx->error_line = ctx->line;
	ctx->expr_boundary = 0;
	ctx->linePos = lpos;

	return lpos;
}

void exec_cmd_cont(struct Context *ctx)
{
	//TODO: Broken
	ctx->linePos++;
	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
	ctx->running = true;

}

void exec_cmd_dim(struct Context *ctx)
{
	unsigned char name[VAR_NAMESZ];

	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
	ctx->linePos = get_symbol(ctx->tokenized_line, ctx->linePos, name);
	exec_expr(ctx);
	//var_add_update(ctx, name, 0, ctx->dstack[ctx->dsptr--], 0);
}

void exec_cmd_end(struct Context *ctx)
{
	ctx->running = false;
}

void exec_cmd_for(struct Context *ctx)
{
	double startval = 0;
	double endval = 0;
	double stepval = 0;
	TokenType tkn;
	unsigned char varname[5] = { 0 };

	tkn.token = (unsigned char*)malloc(sizeof(unsigned char) * 160);

	ctx->linePos = get_token(ctx->tokenized_line, ctx->linePos, &tkn);
	if (tkn.type == TOKEN_TYPE_FLT_VAR || tkn.type == TOKEN_TYPE_INT_VAR)
	{
		strcpy((char*)varname, (char*)tkn.token);

		ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

		if (ensure_token(ctx->tokenized_line[ctx->linePos], 1, '='))
		{
			ctx->linePos = ignore_space(ctx->tokenized_line, ++ctx->linePos);
			ctx->linePos = exec_expr(ctx);
			startval = atof((char *)ctx->expression_result);

			if (varname[length(varname) - 1] == '%')
				var_add_update_int(ctx, varname, startval);
			else
				var_add_update_float(ctx, varname, startval);

			ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
			if (ctx->tokenized_line[ctx->linePos] == TOKEN_TO)
			{
				ctx->linePos++;
				ctx->linePos = exec_expr(ctx);
				endval = atof((char *)ctx->expression_result);

				double step = 1;
				ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
				if (ctx->tokenized_line[ctx->linePos] == TOKEN_STEP)
				{
					ctx->linePos = ignore_space(ctx->tokenized_line, ++ctx->linePos);
					ctx->linePos = exec_expr(ctx);
					step = atof((char *)ctx->expression_result);
					ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
				}

				if (ctx->linePos == -1 || ctx->tokenized_line[ctx->linePos] == ':')
				{
					if (ctx->tokenized_line[ctx->linePos] == ':')
						ctx->linePos++;

					struct forstackitem fsi;
					fsi.var = (unsigned char *)malloc(sizeof(unsigned char)* length(varname) + 1);
					strcpy((char *)fsi.var, (char *)varname);
					fsi.endval = endval;
					fsi.linenumber = ctx->line;
					fsi.linepos = ctx->linePos;
					fsi.step = step;

					// check if this is already on the for stack. If so, update the values.
					bool found = false;
					for (int x = 0; x < forstackidx; x++)
					{
						if (strcmp((char*)forstack[x].var, (char*)fsi.var) == 0)
						{
							found = true;
							forstack[x].endval = fsi.endval;
							forstack[x].linenumber = fsi.linenumber;
							forstack[x].linepos = fsi.linepos;
							forstack[x].step = fsi.step;
							break;
						}
					}

					// if not on for stack, add it
					if (found == false)
						forstack[forstackidx++] = fsi;
				}
				else
				{
					ctx->error = ERR_SYNTAX;
					ctx->error_line = ctx->line;
				}
			}
			else
			{
				ctx->error = ERR_SYNTAX;
				ctx->error_line = ctx->line;
			}

		}
		else
		{
			ctx->error = ERR_SYNTAX;
			ctx->error_line = ctx->line;
		}

	}
	else
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
	}

	free(tkn.token);
}

void exec_cmd_gosub(struct Context *ctx)
{
	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

	if (ensure_token(ctx->tokenized_line[ctx->linePos], DIGIT_LIST_COUNT, DIGIT_LIST))
	{
		exec_expr(ctx);

		// store line number and next statement pos on stack
		ctx->linePos = next_statement(ctx);

		ctx->cstack[++ctx->csptr] = ctx->line;
		ctx->cstack[++ctx->csptr] = ctx->linePos;

		// set jump flag
		ctx->jmpline = atoi((char *)ctx->expression_result);
		ctx->linePos = 0;

		if (ll_find(ctx->jmpline) == NULL)
		{
			ctx->error = ERR_UNDEFSTATEMENT;
			ctx->error_line = ctx->line;
			return;
		}
	}
	else
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
	}
}

void exec_cmd_goto(struct Context *ctx)
{
	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

	if (ensure_token(ctx->tokenized_line[ctx->linePos], DIGIT_LIST_COUNT, DIGIT_LIST))
	{
		exec_expr(ctx);
		
		ctx->jmpline = atoi((char *)ctx->expression_result);
		ctx->linePos = 0;
				
		if (ll_find(ctx->jmpline) == NULL)
		{
			ctx->error = ERR_UNDEFSTATEMENT;
			ctx->error_line = ctx->line;
			return;
		}
	}
	else
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
	}
}

void exec_cmd_if(struct Context *ctx)
{
	int ctr = 0;
	unsigned char expr[200] = { 0 };

	while (true)
	{
		if (ctx->tokenized_line[ctx->linePos] == 0)
		{
			ctx->error = ERR_SYNTAX;
			ctx->error_line = ctx->line;
			break;
		}
		
		if (ctx->tokenized_line[ctx->linePos] == TOKEN_THEN)
			break;

		expr[ctr++] = ctx->tokenized_line[ctx->linePos++];
	}

	expr[ctr] = 0;
	ctx->expr_boundary = ctx->linePos-1;
	ctx->linePos = ctx->linePos - ctr;
	ctx->linePos = exec_expr(ctx);

	if (ctx->error == ERR_NONE)
	{
		if (atoi((char *)ctx->expression_result) == -1)
		{
			ctx->linePos++;
			return;
		}
		else
			ctx->linePos = -1;
	}
}

void exec_cmd_input(struct Context *ctx)
{
	unsigned char name[VAR_NAMESZ+3], ch;
	float value = 0;
	int j;
	int promptCtr = 0;
	unsigned char prompt[160] = { 0 };

	if (ctx->running == false)
	{
		ctx->error = ERR_ILLEGAL_DIRECT;
		ctx->error_line = -1;
		return;
	}

	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

	if (ctx->linePos == -1 || ensure_token(ctx->tokenized_line[ctx->linePos], 1, ':'))
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
		return;
	}

	if (ctx->tokenized_line[ctx->linePos] == '\"')
	{
		ctx->linePos++;

		while (ctx->tokenized_line[ctx->linePos] != '\"')
			prompt[promptCtr++] = ctx->tokenized_line[ctx->linePos++];

		prompt[promptCtr] = 0;

		/* ignore quotation mark */
		ctx->linePos++;

		/* ensure semicolon */
		if (ensure_token(ctx->tokenized_line[ctx->linePos], 1, ';'))
		{
			printf("%s", prompt);
			ctx->linePos++;
		}
		else
		{
			ctx->error = ERR_SYNTAX;
			ctx->error_line = ctx->line;
			return;
		}
	}

	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
	ctx->linePos = get_symbol(ctx->tokenized_line, ctx->linePos, name);

	ch = 0;

	unsigned char buffer[160] = { 0 };
	int bufferctr = 0;

again:
	printf("? ");
	
	while (true)
	{
		char ch = getchar();

		if (ch != 0)
		{
			//putchar(c);

			if (ch == 27)
			{
				ctx->error = ERR_BREAK;
				ctx->error_line = ctx->line;
				return;
			}

			if (ch == '\n')
			{
				buffer[bufferctr] = '\0';
				break;
			}
			else if (ch == 8)
			{
				if (bufferctr > 0)
					bufferctr--;
			}
			else
				buffer[bufferctr++] = ch;
		}
	}

	if (name[length(name) - 1] == '$')
	{
		var_add_update_string(ctx, name, buffer, length(buffer));
	}
	else
	{
		bool numeric = true;
		int x = 0;
		for (x = 0; x < length(buffer); x++)
			if (!ISDIGIT(buffer[x]) && buffer[x] != '.')
				numeric = false;

		if (numeric == false)
		{
			printf("?Redo from start\n");
			bufferctr = 0;
			goto again;
		}

		get_float(buffer, 0, &value);
		var_add_update_float(ctx, name, value);
	}

	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

	// anything after expression must be end of line or colon
	if (ctx->linePos != -1 && ctx->tokenized_line[ctx->linePos] != ':')
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
		return;
	}
}

void exec_cmd_let(struct Context *ctx)
{
	unsigned char name[VAR_NAMESZ+2]; // 2 chars, var type, \0
	int j = 0;
	int len = 0;

	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
	ctx->linePos = get_symbol(ctx->tokenized_line, ctx->linePos, name);
	ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

	if (ctx->tokenized_line[ctx->linePos] == '[')
	{
		ctx->linePos = exec_expr(ctx);
		ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
		j = ctx->dstack[ctx->dsptr--];
	}

	int varFound = 0;
	for (int j = 0; j < ctx->var_count; j++)
	{
		if (compare(ctx->vars[j].name, name))
		{
			varFound = 1;
			break;
		}
	}
	if (varFound == 0)
	{
		if (name[length(name) - 1] == '$')
			var_add_update_string(ctx, name, (unsigned char *)"\0", 1);
		else
		if (name[length(name) - 1] == '%')
			var_add_update_int(ctx, name, 0);
		else
			var_add_update_float(ctx, name, 0);
	}

	// anything after token must be equal sign
	if (ensure_token(ctx->tokenized_line[ctx->linePos], 1, '='))
	{
		ctx->linePos++;

		//if (name[length(name) - 1] == '$')
		//	exec_strexpr(ctx, &len);
		//else
		exec_expr(ctx);
		
		ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);

		// anything after expression must be end of line or colon
		if (ctx->linePos != -1 && ctx->tokenized_line[ctx->linePos] != ':')
		{
			ctx->error = ERR_SYNTAX;
			ctx->error_line = ctx->line;
			return;
		}

		if (name[length(name) - 1] == '$')
			var_add_update_string(ctx, name, ctx->expression_result, length(ctx->expression_result));
		else
		if (name[length(name) - 1] == '%')
			var_add_update_int(ctx, name, atoi((char *)ctx->expression_result));
		else
			var_add_update_float(ctx, name, atof((char *)ctx->expression_result));
	}
	else
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
		return;
	}
}

void exec_cmd_list(struct Context*ctx)
{
	ll_sort();

	struct node *ptr = ll_gethead();
	printf("\n");

	//start from the beginning
	while (ptr != NULL)
	{
		char lnbuff[10] = { 0 };
		snprintf(lnbuff, sizeof(lnbuff), "%d", ptr->linenum);
		printf("%s %s\n", lnbuff, ptr->data);
		ptr = ptr->next;
	}
}

void exec_cmd_load(struct Context*ctx)
{
	const char *file = "10 PRINT A\n20GOTO20";
	unsigned char buffer[160];
	int bufferCtr = 0;
	int linenum = 0;
	char b[2];
	int dataPtr = 0;

	// new cmd
	while (!ll_isEmpty())
		ll_deleteFirst();

		printf("Loading\n");

		while (true)
		{
			b[0] = *file++;
			if (b[0] != 0)
			{
				if (b[0] != '\n')
					buffer[bufferCtr++] = b[0];
				else
				{
					if (bufferCtr != 0)
					{
						buffer[bufferCtr] = 0;

						dataPtr = get_int(buffer, 0, &linenum);
						dataPtr = ignore_space(buffer, dataPtr);
						memcpy(buffer, buffer + dataPtr, length(buffer)+dataPtr);

						printf("%d - %s\n", linenum, buffer);

						unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * length(buffer) + 1);
						memcpy(data, buffer, length(buffer) + 1);
						ll_insertFirst(linenum, data);
						bufferCtr = 0;
					}
				}
			}
			else
			{
				if (bufferCtr != 0)
				{
					buffer[bufferCtr] = 0;
					dataPtr = get_int(buffer, 0, &linenum);
					dataPtr = ignore_space(buffer, dataPtr);
					memcpy(buffer, buffer + dataPtr, length(buffer) + dataPtr);

					printf("*%d - %s\n", linenum, buffer);

					unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * length(buffer) + 1);
					memcpy(data, buffer, length(buffer) + 1);
					ll_insertFirst(linenum, data);
					bufferCtr = 0;
				}
				break;
			}
		};

		ll_sort();

}

void exec_cmd_new(struct Context *ctx)
{
	while (!ll_isEmpty())
		ll_deleteFirst();

	var_clear_all(ctx);
}

void exec_cmd_next(struct Context *ctx)
{
	TokenType tkn;
	double currentvalue = 0;
	unsigned char varname[10];
	bool found = false;

	if (forstackidx == 0)
	{
		// next without for
		ctx->error = ERR_NEXT_WO_FOR;
		ctx->error_line = ctx->line;
		return;
	}

	tkn.token = (unsigned char*)malloc(sizeof(unsigned char) * 160);

	ctx->linePos = get_token(ctx->tokenized_line, ctx->linePos, &tkn);
	if (tkn.type == TOKEN_TYPE_FLT_VAR || tkn.type == TOKEN_TYPE_INT_VAR || tkn.type == TOKEN_TYPE_EOL || tkn.type == TOKEN_TYPE_EOS)
	{
		if (tkn.type == TOKEN_TYPE_FLT_VAR || tkn.type == TOKEN_TYPE_INT_VAR)
		{
			// code is NEXT with a variable, so we get the value of the variable (doesnt matter if it doesnt exist yet)
			for (int x = 0; x < ctx->var_count; x++)
			{
				if (strcmp((char *)tkn.token, (char*)ctx->vars[x].name) == 0)
				{
					currentvalue = *((float*)ctx->vars[x].location);
					break;
				}
			}
		}
		else
		{
			// backup if not EOL
			if(tkn.type == TOKEN_TYPE_EOS)	ctx->linePos--;

			// code is NEXT without a variable, so we use the topmost variable on for stack
			strcpy((char *)tkn.token, (char*)forstack[forstackidx-1].var);
			for (int x = 0; x < ctx->var_count; x++)
			{
				if (strcmp((char *)tkn.token, (char*)ctx->vars[x].name) == 0)
				{
					currentvalue = *((float*)ctx->vars[x].location);
					break;
				}
			}
		}

		for (int x = 0; x < forstackidx; x++)
		{
			// find the NEXT variable
			if (strcmp((char *)forstack[x].var, (char *)tkn.token) == 0)
			{
				forstackidx = x+1;
				found = true;

				// if we have not exceeded the value
				if ((forstack[x].step > 0 && currentvalue + forstack[x].step <= forstack[x].endval) || 
					(forstack[x].step < 0 && currentvalue + forstack[x].step >= forstack[x].endval))
				{
					// increment variable based on step value
					var_add_update_float(ctx, tkn.token, currentvalue + forstack[x].step);

					// get and retokenize the line
					ctx->original_line = ll_find(forstack[x].linenumber)->data;
					free(ctx->tokenized_line);
					unsigned char* tokenized_line = (unsigned char*)malloc(sizeof(unsigned char) * 160);
					tokenize(ctx, ctx->original_line, tokenized_line);
					ctx->tokenized_line = tokenized_line;

					// reset the line position to iterate again
					ctx->jmpline = ll_find(forstack[x].linenumber)->linenum;
					ctx->linePos = forstack[x].linepos;
					ctx->line = ll_find(forstack[x].linenumber)->linenum;
				}
				else
				{
					// if end of for loop, pop the for stack
					forstackidx--;

					ctx->linePos = get_token(ctx->tokenized_line, ctx->linePos, &tkn);

					if (tkn.type != TOKEN_TYPE_EOL && tkn.type != TOKEN_TYPE_EOS)
					{
						ctx->error = ERR_SYNTAX;
						ctx->error_line = ctx->line;
					}
				}

				break;
			}
		}

		if (found == false)
		{
			// next without for
			ctx->error = ERR_NEXT_WO_FOR;
			ctx->error_line = ctx->line;
		}
	}
	else
	{
		ctx->error = ERR_SYNTAX;
		ctx->error_line = ctx->line;
	}

	free(tkn.token);
}

void exec_cmd_print(struct Context *ctx)
{
	bool eol = true;
	bool quoteMode = false;
	unsigned char val[200];

	while (ctx->tokenized_line[ctx->linePos] != 0 && ctx->tokenized_line[ctx->linePos] != ':')
	{
		ctx->linePos = ignore_space(ctx->tokenized_line, ctx->linePos);
		ctx->linePos = exec_expr(ctx);

		if (ctx->error == ERR_NONE)
		{
			if (ctx->expression_type == DATATYPE_FLOAT)
				printf(" %g ", atof((char *)ctx->expression_result));
			else
			if (ctx->expression_type == DATATYPE_INT)
				printf(" %d ", atoi((char *)ctx->expression_result));
			else
			if (ctx->expression_type == DATATYPE_STRING)
				printf("%s", ctx->expression_result);

			if (ctx->tokenized_line[ctx->linePos] == ';')
			{
				ctx->linePos++;
				eol = false;
			}

			if (ctx->tokenized_line[ctx->linePos] == ',')
			{
				ctx->linePos++;
				printf("     ");
				eol = false;
			}

			if (eol) putchar('\n');
		}
	}
}

void exec_cmd_rem(struct Context *ctx)
{
	// skip this line (do nothing)
	ctx->linePos = -1;
	return;
}

void exec_cmd_return(struct Context *ctx)
{
	if (ctx->csptr == 0)
	{
		ctx->error = ERR_RTRN_WO_GSB;
		ctx->error_line = ctx->line;
		return;
	}

	// release current line resource
	free(ctx->tokenized_line);
	
	// Get previous line pos and line number from stack
	ctx->linePos = ctx->cstack[ctx->csptr--];
	ctx->line = ll_find(ctx->cstack[ctx->csptr])->linenum;
	ctx->original_line = ll_find(ctx->cstack[ctx->csptr--])->data;
	
	// retokenize the line
	unsigned char* tokenized_line = (unsigned char*)malloc(sizeof(unsigned char)*160);
	tokenize(ctx, ctx->original_line, tokenized_line);

	ctx->tokenized_line = tokenized_line;
	ctx->jmpline = ctx->line;
}

void exec_cmd_run(struct Context *ctx)
{
	exec_program(ctx);
	ctx->linePos = -1;
	return;
}

void exec_cmd_save(struct Context*ctx)
{
	char fnbuffer[15] = { 0 };
    printf("Saving\n");

		ll_sort();
		struct node *ptr = ll_gethead();

		//start from the beginning
		while (ptr != NULL)
		{
			char lnbuff[160] = { 0 };
			snprintf(lnbuff, sizeof(lnbuff), "%d %s\n", ptr->linenum, ptr->data);
			puts(lnbuff);
			ptr = ptr->next;
		}

}

void exec_cmd_stop(struct Context*ctx)
{
	ctx->error = ERR_BREAK;
	ctx->error_line = ctx->line;
	return;
}

void var_clear_all(struct Context *ctx)
{
	for (int j = 0; j < ctx->var_count; j++)
	{
		ctx->vars[j].name[0] = 0;
		ctx->vars[j].type = DATATYPE_UNDEFINED;
		free(ctx->vars[j].location);
	}

	ctx->var_count = 0;
}

void var_add_update_int(struct Context *ctx, const unsigned char *key, int value)
{
	int j;

	// find and update the variable
	for (j = 0; j < ctx->var_count; j++)
	{
		if (compare(ctx->vars[j].name, key) && ctx->vars[j].type == DATATYPE_INT)
		{
			free(ctx->vars[j].location);
			ctx->vars[j].location = (int *)malloc(sizeof(int));
			*((int*)ctx->vars[j].location) = value;
			return;
		}
	}
	
	// variable not found.  add it
	ctx->var_count = j+1;
	strcpy((char *)ctx->vars[ctx->var_count-1].name, (char *)key);
	ctx->vars[ctx->var_count-1].location = (int *)malloc(sizeof(int));
	ctx->vars[ctx->var_count-1].type = DATATYPE_INT;
	*((int*)ctx->vars[ctx->var_count-1].location) = value;
	
}

void var_add_update_float(struct Context *ctx, const unsigned char *key, float value)
{
	int j;

	// find and update the variable
	for (j = 0; j < ctx->var_count; j++)
	{
		if (compare(ctx->vars[j].name, key) && ctx->vars[j].type == DATATYPE_FLOAT)
		{
			free(ctx->vars[j].location);
			ctx->vars[j].location = (float *)malloc(sizeof(float));
			*((float*)ctx->vars[j].location) = value;
			return;
		}
	}

	// variable not found.  add it
	ctx->var_count = j + 1;
	strcpy((char *)ctx->vars[ctx->var_count - 1].name, (char *)key);
	ctx->vars[ctx->var_count - 1].location = (float *)malloc(sizeof(float));
	ctx->vars[ctx->var_count - 1].type = DATATYPE_FLOAT;
	*((float*)ctx->vars[ctx->var_count - 1].location) = value;

}

void var_add_update_string(struct Context *ctx, const unsigned char *key, unsigned char* value, int length)
{
	int j;

	// find and update the variable
	for (j = 0; j < ctx->var_count; j++)
	{
		if (compare(ctx->vars[j].name, key) && ctx->vars[j].type == DATATYPE_STRING)
		{
			free(ctx->vars[j].location);
			ctx->vars[j].location = (unsigned char*)malloc(sizeof(unsigned char*) * length+1);

			for(int x=0;x<length;x++)
				*((char*)ctx->vars[j].location+x) = value[x];

			*((char*)ctx->vars[j].location + length) = 0;
			return;
		}
	}

	// variable not found.  add it
	ctx->var_count = j + 1;
	strcpy((char *)ctx->vars[ctx->var_count - 1].name, (char *)key);
	ctx->vars[ctx->var_count - 1].location = (unsigned char *)malloc(sizeof(unsigned char*) * length + 1);
	ctx->vars[ctx->var_count - 1].type = DATATYPE_STRING;
	
	for (int x = 0; x<length; x++)
		*((unsigned char*)ctx->vars[j].location + x) = value[x];

	*((unsigned char*)ctx->vars[j].location + length) = 0;

}

// sbparse

int get_symbol(const unsigned char *s, int i, unsigned char *t)
{
	int done = 0;
	int type = 0;

	if (s[i] > 127) type = 0; // cmd / func
	if (ISALPHA(s[i])) type = 1; //var
	if (s[i] == '\"') 
	{
		*(t++) = s[i++];
		type = 2; // str
	}

	while (true)
	{
		if (s[i] == 0)
			break;

		if(type == 0)
		{
			if (s[i] > 127)
				*(t++) = s[i++];
			break;
		}
		else
		if (type == 1)
		{
			if (ISALPHA(s[i]))
				*(t++) = s[i++];

			// string
			if (s[i] == '$')
				*(t++) = s[i++];

			// int
			if (s[i] == '%')
				*(t++) = s[i++];

			break;
		}
		else
		if(type == 2)
		{
			if (s[i] != '\"')
				*(t++) = s[i++];
			else
			{
				*(t++) = s[i++];
				break;
			}
				
		}
	}

	*t = 0;
	return i;
}

int get_token(const unsigned char *s, int i, TokenType* token)
{
	int done = 0;
	token->type = TOKEN_TYPE_UNKNOWN;

	if (i == -1 || i > length(s))
	{
		token->type = TOKEN_TYPE_EOL;
		return i;
	}

	while (s[i] == ' ')
		i++;

	if (s[i] > 127)
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_KEYWORD;
		if (s[i] > 179 && s[i] < 196) token->type = TOKEN_TYPE_FLT_FUNC;
		if (s[i] == 196 || (s[i] > 197 && s[i] < 203)) token->type = TOKEN_TYPE_STR_FUNC;
		token->token[ctr++] = s[i++];
		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == '+' || s[i] == '-' || s[i] == '-' || s[i] == '*' || s[i] == '/' || s[i] == '(' || s[i] == ')')
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_OPERATOR;
		token->token[ctr++] = s[i++];
		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == '=' || s[i] == '>' || s[i] == '<')
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_EQUALITY;
		token->token[ctr++] = s[i++];

		if ((s[i - 1] == '<' || s[i - 1] == '>') && s[i] == '=')
			token->token[ctr++] = s[i++];
		else
			if (s[i - 1] == '<' && s[i] == '>')
				token->token[ctr++] = s[i++];
		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == '\"')
	{
		token->type = TOKEN_TYPE_STR_LIT;
		int ctr = 0;
		while (s[i] != 0)
		{
			token->token[ctr++] = s[i++];
			if (s[i - 1] == '\"')
				break;
		}
		token->token[ctr] = 0;
		return i;
	}
	if (ISALPHA(s[i]))
	{
		int ctr = 0;
		while (s[i] != 0 && ISALPHA(s[i]))
		{
			token->token[ctr++] = s[i++];
		}

		if (s[i] == '$')
		{
			token->type = TOKEN_TYPE_STR_VAR;
			token->token[ctr++] = s[i++];
		}
		else
		if (s[i] == '%')
		{
			token->type = TOKEN_TYPE_INT_VAR;
			token->token[ctr++] = s[i++];
		}
		else
		{
			token->type = TOKEN_TYPE_FLT_VAR;
		}
		token->token[ctr] = 0;
		return i;
	}
	if (ISDIGIT(s[i]) || s[i] == '.')
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_INT_LIT;

		while (s[i] != 0 && (ISDIGIT(s[i]) || s[i] == '.'))
		{
			if (s[i] == '.')
				token->type = TOKEN_TYPE_FLT_LIT;
			token->token[ctr++] = s[i++];
		}

		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == ';' || s[i] == ',')
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_DELIMITER;
		token->token[ctr++] = s[i++];
		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == ':')
	{
		int ctr = 0;
		token->type = TOKEN_TYPE_EOS;
		token->token[ctr++] = s[i++];
		token->token[ctr] = 0;
		return i;
	}
	if (s[i] == 0)
	{
		token->type = TOKEN_TYPE_EOL;
		i = -1;
		return i;
	}

	return i;
}

int get_int(const unsigned char *s, int i, int *num)
{
	*num = 0;
	while (s[i] && ISDIGIT(s[i]))
	{
		*num *= 10;
		*num += s[i++] - '0';
	}
	return i;
}

int get_float(const unsigned char* s, int i, float *fv)
{
	float rez = 0, fact = 1;

	if (i == 0 && s[i] == '-')
	{
		i++;
		fact = -1;
	};

	for (int point_seen = 0; ISDIGIT(s[i]) || (s[i] == '.' && point_seen == 0); i++)
	{
		if (s[i] == '.' && point_seen == 0)
		{
			point_seen = 1;
			continue;
		}

		int d = s[i] - '0';

		if (d >= 0 && d <= 9)
		{
			if (point_seen) fact /= 10.0f;
			rez = rez * 10.0f + (float)d;
		};
	};

	*fv = rez * fact;
	return i;
};

int ignore_space(const unsigned char *s, int i)
{
	while (s[i] && ISSPACE(s[i]))
	{
		i++;
	}

	if (s[i] == 0)
		return -1;
	else
		return i;
}

void to_uppercase(unsigned char *s)
{
	bool toggle = true;
	while (*s)
	{
		if (ISalpha(*s))
		{
			if (toggle)
				*s = *s - 32;
		}
		else
			if (*s == '\"')
			{
				toggle = !toggle;
			}
			else
				if (*s == '\'')
				{
					toggle = false;
				}
				else
					if (*s == '\n')
					{
						toggle = true;
					}
		s++;
	}
}

bool isemptyline(unsigned char *linebuf)
{
	bool emptyline = true;
	int ctr = 0;
	while (ISDIGIT(linebuf[ctr]))
	{
		ctr++;
	}

	while (linebuf[ctr] != '\0')
	{
		if (!ISSPACE(linebuf[ctr]))
		{
			emptyline = false;
			break;
		}
		ctr++;
	}

	return emptyline;
}

bool compare(const unsigned char *a, const unsigned char *b)
{
	if (a == false && b == false) return true;
	if (a == false || b == false) return false;
	while (*a && *b && (*a == *b))
	{
		a++;
		b++;
	}
	if ((*a == '\0') && (*b == '\0')) return true;
	return false;
}

void clear(unsigned char *dst, int size)
{
	for (; size>0; size--, dst++)
	{
		*dst = 0;
	}
}

int length(const unsigned char *s)
{
	int l = 0;
	while (*(s++)) l++;
	return l;
}



void join(unsigned char *dst, const unsigned char *src)
{
	strcpy((char *)(dst + strlen((char *)dst)), (char *)src);
}

void get_input(unsigned char *s)
{
	int inbufferctr = 0;

	while (true)
	{
		char c = getchar();

		if (c != 0)
		{
			//putchar(c);

			if (c == '\n')
			{
				s[inbufferctr] = '\0';
				break;
			}
			else if (c == 8)
			{
				if (inbufferctr > 0)
					inbufferctr--;
			}
			else
				s[inbufferctr++] = c;
		}
	}
}

int next_statement(struct Context *ctx)
{
	int ctr = ctx->linePos;
	int quoteMode = 0;

	while (true)
	{
		if (ctx->tokenized_line[ctr] == '\0' || (ctx->tokenized_line[ctr] == ':' && quoteMode == 0))
			break;

		if (ctx->tokenized_line[ctr] == '\"')
		{
			if (quoteMode == 0)
				quoteMode = 1;
			else
				quoteMode = 0;
		}
		ctr++;
	}

	if (ctx->tokenized_line[ctr] == ':')
		ctr++;
	else
		ctr = -1;

	return ctr;
}

void tokenize(struct Context* ctx, const unsigned char* input, unsigned char *output)
{
	int i = 0, o = 0, tempStackPtr = 0;
	bool quoteMode = false;
	bool foundCmd = false;
	unsigned char tempStack[160] = { 0 };

	while (true)
	{
		if (input[i] == 0 || input[i] == ':')
		{
			if (tempStackPtr != 0)
			{
				for (int x = 0; x < tempStackPtr; x++)
					output[o++] = tempStack[x];
				tempStackPtr = 0;
			}

			output[o] = input[i];

			if (input[i] == 0)
				break;
			else
			{
				i++;
				o++;
				continue;
			}
		}

		if (quoteMode == false)
		{
			if (ISALPHA(input[i]) || (input[i] == '$'))
			{
				// push to the stack
				tempStack[tempStackPtr++] = input[i++];
				tempStack[tempStackPtr] = 0;

				for (int j = 0; j < CMD_COUNT; j++)
				{
					if (compare(tempStack, ctx->cmds[j].name))
					{
						output[o++] = ctx->cmds[j].token;
						tempStackPtr = 0;
						break;
					}
				}
			}
			else
			{
				if (input[i] == '\"')
					quoteMode = true;

				tempStack[tempStackPtr++] = input[i++];

				for (int x = 0; x < tempStackPtr; x++)
					output[o++] = tempStack[x];
				
				tempStackPtr = 0;
			}
		}
		if (quoteMode == true)
		{
			if(input[i] == '\"')
				quoteMode = false;
			
			output[o++] = input[i++];
		}
	};

	
	to_uppercase(output);
}

bool ensure_token(unsigned char c, int tokenCount, ...)
{
	// set up variable argument list
	va_list ap;
	va_start(ap, tokenCount);

	for (int x = 0; x < tokenCount; x++)
	{
		unsigned char t = (unsigned char)va_arg(ap, int);
		if (c == t)
		{
			va_end(ap);
			return true;
		}
	}

	va_end(ap);
	return false;
}
