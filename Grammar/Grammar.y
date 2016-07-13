%defines
%debug
%pure-parser
%error-verbose

%lex-param {quex::GeneratedLexer  *qlex}
%parse-param {quex::GeneratedLexer  *qlex } 
%parse-param { Ast::GlobalStatements** stmts }

%{
/* C declarations */
#include <cstdlib>
#include <string>
//#include "Grammar.h"

#define YYDEBUG 1

union YYSTYPE;

namespace quex
{
	class GeneratedLexer;
}

namespace Ast
{
	class GlobalStatements;
}

namespace quex
{
	class GeneratedLexer;
}

int yylex(YYSTYPE *yylval, quex::GeneratedLexer *qlex);
void yyerror(quex::GeneratedLexer *qlex, Ast::GlobalStatements** stmts, const std::string& m);

using namespace Ast;

%}


%code requires {
	#include <Node.h>
	#include <Expressions.h>
	#include <Operations.h>
	#include <Classes.h>
	#include <Statements.h>
	#include <Primitives.h>
	#include <TypeInfo.h>
}

%union
{
	std::string* id;
	Ast::VisibilityNode* vis_node;
	Ast::GlobalStatements* gstmts_node;
	Ast::GlobalStatement* gstmt_node;
	Ast::NamespaceDeclaration* nmsp_node;
	Ast::Reference* ref_node;
	Ast::TypeSpecifier* type_node;
	Ast::ConstantExpression* prim_node;
	Ast::ClassMemberDeclaration* clss_mem_node;
	Ast::Argument* arg_node;
	Ast::ArgumentList* arg_list;
	Ast::FunctionDeclaration* fun_node;
	Ast::ConstructorDeclaration* ctor_node;
	Ast::DestructorDeclaration* dtor_node;
	Ast::ClassStatementList* clss_stmts_node;
	Ast::ClassStatement* clss_stmt_node;
	Ast::ClassDeclaration* clss_decl_node;
	Ast::LineStatement* ln_stmt_node;
	Ast::LineStatements* ln_stmts_node;
	Ast::ScopedStatements* scope_node;
	Ast::Expression* expr;
	Ast::ExpressionList* expr_list;
	Ast::Modifier* mod_node;
	Ast::TypeAndIdentifier* type_and_id;
	Ast::AssignFrom* assign_from_node;
	Ast::AssignFromSingle* assign_single_node;
};

%type <vis_node> visibility;
%type <gstmts_node> global_statements;
%type <gstmt_node> global_statement;
%type <nmsp_node> namespace_declaration;
%type <ref_node> reference;
%type <ref_node> single_identifier;
%type <type_node> type;
%type <type_node> primitive_type;
%type <clss_mem_node> class_member;
%type <arg_node> argument;
%type <arg_list> argument_list;
%type <fun_node> function_declaration;
%type <ctor_node> constructor;
%type <dtor_node> destructor;
%type <clss_stmts_node> class_statement_list;
%type <clss_stmt_node> class_statement;
%type <clss_decl_node> class_declaration;
%type <ln_stmt_node> line_statement;
%type <ln_stmts_node> line_statements;
%type <ln_stmt_node> if_statement;
%type <ln_stmt_node> while_statement;
%type <ln_stmt_node> break_statement;
%type <assign_from_node> assign_from;
%type <assign_single_node> assign_from_single;
%type <ln_stmt_node> assignment;
%type <ln_stmt_node> return_statement;
%type <scope_node> scoped_statement_list;
%type <expr> expression;
%type <expr> argument_expression;
%type <expr> binary_arith_expression;
%type <expr> unary_arith_expression;
%type <expr> binary_comparison_expression;
%type <expr> logical_expression;
%type <expr> new_expression;
%type <expr> function_call;
%type <expr> print_statement;
%type <prim_node> number_literal;
%type <prim_node> bool_literal;
%type <prim_node> char_literal;
%type <prim_node> string_literal;
%type <prim_node> literal;
%type <ln_stmt_node> valid_expression_as_statement;
%type <mod_node> modifier;
%type <mod_node> modifier_list;

/*Bison declarations*/

/* Primitives */
%token TKN_TYPE_INT
%token TKN_TYPE_UINT
%token TKN_TYPE_INT64
%token TKN_TYPE_UINT64
%token TKN_TYPE_FLOAT
%token TKN_TYPE_FLOAT64
%token TKN_TYPE_CHARBYTE
%token TKN_TYPE_CHAR
%token TKN_TYPE_BYTE
%token TKN_TYPE_BOOL
%token TKN_TYPE_STRING

%token TKN_CONSTANT_INT
%token TKN_CONSTANT_FLOAT
%token TKN_CONSTANT_BOOL_TRUE
%token TKN_CONSTANT_BOOL_FALSE
%token TKN_CONSTANT_STRING
%token TKN_CONSTANT_CHAR

/* This should eventually be removed. This is just for debugging at this piont*/
%token TKN_PRINT

/* Operators */
%token TKN_ARITHMETIC_OPERATOR_INCREMENT;
%token TKN_ARITHMETIC_OPERATOR_DECREMENT;
%token TKN_ARITHMETIC_OPERATOR_PLUS;
%token TKN_ARITHMETIC_OPERATOR_MINUS;
%token TKN_ARITHMETIC_OPERATOR_TIMES;
%token TKN_ARITHMETIC_OPERATOR_DIVIDE;
%token TKN_BITWISE_SHIFT_LEFT_OPERATOR;
%token TKN_BITWISE_SHIFT_RIGHT_OPERATOR;
%token TKN_ARITHMETIC_OPERATOR_REMAINDER;
%token TKN_COMPARISON_OPERATOR_GREATER_THAN_OR_EQUAL_TO;
%token TKN_COMPARISON_OPERATOR_LESS_THAN_OR_EQUAL_TO;
%token TKN_COMPARISON_OPERATOR_GREATER_THAN;
%token TKN_COMPARISON_OPERATOR_LESS_THAN;
%token TKN_COMPARISON_OPERATOR_EQUAL_TO;
%token TKN_COMPARISON_OPERATOR_NOT_EQUAL_TO;
%token TKN_LOGICAL_OPERATOR_AND;
%token TKN_LOGICAL_OPERATOR_OR;
%token TKN_LOGICAL_OPERATOR_NEGATE;
%token TKN_BITWISE_OPERATOR_AND;
%token TKN_BITWISE_OPERATOR_OR;
%token TKN_BITWISE_OPERATOR_XOR;
%token TKN_BITWISE_OPERATOR_COMPLEMENT;
%token TKN_OPERATOR_ASSIGN_TO;

/* Identifier */
%token TKN_IDENTIFIER

/* Reserved characters */
%token TKN_SEMICOLON
%token TKN_BRACKET_OPEN;
%token TKN_BRACKET_CLOSE;
%token TKN_PAREN_OPEN;
%token TKN_PAREN_CLOSE;
%token TKN_IF;
%token TKN_ELSE;
%token TKN_LET;
%token TKN_WHILE;
%token TKN_DO;
%token TKN_BREAK;
%token TKN_NAMESPACE;
%token TKN_NEW;
%token TKN_CLASS;
%token TKN_FUNCTION;
%token TKN_RETURN;
%token TKN_COMMA;
%token TKN_PERIOD;
%token TKN_PUBLIC;
%token TKN_PRIVATE;
%token TKN_PROTECTED;
%token TKN_STATIC;

/* Operator precedence */
%left TKN_LOGICAL_OPERATOR_AND TKN_LOGICAL_OPERATOR_OR 
%left TKN_COMPARISON_OPERATOR_GREATER_THAN_OR_EQUAL_TO TKN_COMPARISON_OPERATOR_LESS_THAN_OR_EQUAL_TO 
	TKN_COMPARISON_OPERATOR_GREATER_THAN TKN_COMPARISON_OPERATOR_LESS_THAN 
	TKN_COMPARISON_OPERATOR_EQUAL_TO TKN_COMPARISON_OPERATOR_NOT_EQUAL_TO TKN_LOGICAL_OPERATOR_NEGATE
%left TKN_ARITHMETIC_OPERATOR_PLUS TKN_ARITHMETIC_OPERATOR_MINUS
%left TKN_ARITHMETIC_OPERATOR_TIMES TKN_ARITHMETIC_OPERATOR_DIVIDE
%left TKN_PAREN_OPEN TKN_PAREN_CLOSE

%glr-parser

%%
/*Grammar rules*/
program: 
	  global_statements
	  {
		*stmts = $1;
	  };

visibility :
	  TKN_PUBLIC
	  {
		$$ = new VisibilityNode(Visibility::PUBLIC);
	  }
	| TKN_PRIVATE
	  {
		$$ = new VisibilityNode(Visibility::PRIVATE);
	  }
	| TKN_PROTECTED
	  {
		$$ = new VisibilityNode(Visibility::PROTECTED);
	  }
	| /*default*/
	  {
		$$ = new VisibilityNode(Visibility::PUBLIC);
	  };

global_statement :
	  class_declaration
	| namespace_declaration;

global_statements :
	  global_statement global_statements 
	  {
		$$ = new GlobalStatements($1, $2);
	  }
	| /*epsilon*/
	  {
		$$ = NULL;
	  };

namespace_declaration :
	  TKN_NAMESPACE single_identifier TKN_BRACKET_OPEN global_statements TKN_BRACKET_CLOSE
	  {
		$$ = new NamespaceDeclaration($2->Id(), $4);
		delete $2;
	  };

single_identifier :
	  TKN_IDENTIFIER
	  {
		$$ = new Reference(*$<id>1);
		delete $<id>1;
	  };

reference : 
	  single_identifier TKN_PERIOD reference
	  {
		$$ = new Reference($1->Id(), $3->Id());
		delete $1;
		delete $3;
	  }
	| single_identifier;

type :
	  TKN_LET
	  {
		$$ = new TypeSpecifier();
	  }
	| reference
	  {
		$$ = new TypeSpecifier($1->Id());
		delete $1;
	  }
	| primitive_type;

primitive_type:
	  TKN_TYPE_INT
	  {
		$$ = new TypeSpecifier(Int32TypeInfo::Get());
	  }
	| TKN_TYPE_UINT
	  {
		$$ = new TypeSpecifier(UInt32TypeInfo::Get());
	  }
	| TKN_TYPE_INT64
	  {
		$$ = new TypeSpecifier(Int64TypeInfo::Get());
	  }
	| TKN_TYPE_UINT64
	  {
		$$ = new TypeSpecifier(UInt64TypeInfo::Get());
	  }
	| TKN_TYPE_FLOAT
	  {
		$$ = new TypeSpecifier(Float32TypeInfo::Get());
	  }
	| TKN_TYPE_FLOAT64
	  {
		$$ = new TypeSpecifier(Float64TypeInfo::Get());
	  }
	| TKN_TYPE_CHARBYTE
	  {
		$$ = new TypeSpecifier(CharByteTypeInfo::Get());
	  }
	| TKN_TYPE_CHAR
	  {
		$$ = new TypeSpecifier(CharTypeInfo::Get());
	  }
	| TKN_TYPE_BYTE
	  {
		$$ = new TypeSpecifier(ByteTypeInfo::Get());
	  }
	| TKN_TYPE_BOOL
	  {
		$$ = new TypeSpecifier(BoolTypeInfo::Get());
	  }
	| TKN_TYPE_STRING
	  {
		$$ = new TypeSpecifier(StringTypeInfo::Get());
	  };

modifier:
	  TKN_STATIC
	  {
		$$ = new Modifier(Modifier::Modifiers::STATIC);
	  };

modifier_list:
	  modifier_list modifier
	  {
		Modifier::Modifiers mods = (Modifier::Modifiers)0;
		if ($1 != NULL)
		{
			mods = $1->Get();
			delete $1;
		}
		if ($2 != NULL)
		{
			mods = (Modifier::Modifiers)((int)mods | (int)$2->Get());
			delete $2;
		}
		$$ = new Modifier((Modifier::Modifiers)mods);
	  }
	| /*epsilon*/
	  {
		$$ = new Modifier(Modifier::Modifiers::NONE);;
	  };

class_member:
	  visibility modifier_list type single_identifier TKN_SEMICOLON
	  {
		$$ = new ClassMemberDeclaration($1->Get(), $2, $3->GetTypeInfo(), $4->Id());
		delete $4;
		delete $3;
		delete $1;
	  }
	| visibility modifier_list type single_identifier TKN_OPERATOR_ASSIGN_TO literal TKN_SEMICOLON
	  {
		$$ = new ClassMemberDeclaration($1->Get(), $2, $3->GetTypeInfo(), $4->Id(), $6);
		delete $4;
		delete $3;
		delete $1;
	  };

argument:
	  type single_identifier
	  {
		$$ = new Argument($1->GetTypeInfo(), $2->Id());
		delete $1;
		delete $2;
	  };

argument_list:
	  argument TKN_COMMA argument_list
	  {
		$$ = new ArgumentList($1, $3)
	  }
	| argument
	  {
		$$ = new ArgumentList($1);
	  }
	| /* epsilon */
	  {
		$$ = NULL;
	  };

function_declaration :
	  visibility modifier_list TKN_FUNCTION TKN_PAREN_OPEN argument_list TKN_PAREN_CLOSE single_identifier TKN_PAREN_OPEN argument_list TKN_PAREN_CLOSE TKN_BRACKET_OPEN line_statements TKN_BRACKET_CLOSE
	  {
		$$ = new FunctionDeclaration($1->Get(), $2, $5, $7->Id(), $9, $12);
		delete $7;
		delete $1;
	  }
	| visibility modifier_list TKN_FUNCTION single_identifier TKN_PAREN_OPEN argument_list TKN_PAREN_CLOSE TKN_BRACKET_OPEN line_statements TKN_BRACKET_CLOSE
	  {
		$$ = new FunctionDeclaration($1->Get(), $2, NULL, $4->Id(), $6, $9);
		delete $4;
		delete $1;
	  };

constructor:
	  visibility single_identifier TKN_PAREN_OPEN argument_list TKN_PAREN_CLOSE TKN_BRACKET_OPEN line_statements TKN_BRACKET_CLOSE
	  {
		$$ = new ConstructorDeclaration($1->Get(), $2->Id(), $4, $7);
		delete $2;
		delete $1;
	  };

destructor:
	  TKN_BITWISE_OPERATOR_COMPLEMENT single_identifier TKN_PAREN_OPEN TKN_PAREN_CLOSE TKN_BRACKET_OPEN line_statements TKN_BRACKET_CLOSE
	  {
		$$ = new DestructorDeclaration($2->Id(), $6);
		delete $2;
	  };

class_statement_list:
	  class_statement class_statement_list
	  {
		$$ = new ClassStatementList($1, $2);
	  }
	| /*epsilon*/
	  {
		$$ = NULL;
	  };

class_statement :
	  class_member
	| constructor
	| destructor
	| function_declaration;

class_declaration :
	  visibility TKN_CLASS single_identifier TKN_BRACKET_OPEN class_statement_list TKN_BRACKET_CLOSE
	  {
		$$ = new ClassDeclaration($1->Get(), $3->Id(), $5);
		delete $3;
		delete $1;
	  };

if_statement : 
	  TKN_IF TKN_PAREN_OPEN expression TKN_PAREN_CLOSE line_statement
	  {
		$$ = new IfStatement($3, $5);
	  }
	| TKN_IF TKN_PAREN_OPEN expression TKN_PAREN_CLOSE line_statement TKN_ELSE line_statement
	  {
		$$ = new IfStatement($3, $5, $7);
	  };

while_statement : 
	  TKN_WHILE TKN_PAREN_OPEN expression TKN_PAREN_CLOSE line_statement
	  {
		$$ = new WhileStatement($3, $5);
	  };

break_statement :
	  TKN_BREAK TKN_SEMICOLON
	  {
		$$ = new BreakStatement();
	  }

assign_from:
	  assign_from_single TKN_COMMA assign_from
	  {
		$$ = new AssignFrom($1, $3);
	  }
	| assign_from_single
	  {
		$$ = new AssignFrom($1);
	  };

assign_from_single:
	  type single_identifier
	  {
		$$ = new DeclareVariable($1->GetTypeInfo(), $2->Id());
		delete $1;
		delete $2;
	  }
	| reference
	  {
		$$ = new AssignFromReference($1->Id());
		delete $1;
	  };

assignment:
	  assign_from TKN_OPERATOR_ASSIGN_TO argument_expression TKN_SEMICOLON
	  {
		$$ = new Assignment($1, $3);
	  };

line_statements : 
	  line_statement line_statements
	  {
		$$ = new LineStatements($1, $2);
	  }
	| /*epsilon*/
	  {
		$$ = NULL;
	  };

scoped_statement_list :
	  TKN_BRACKET_OPEN line_statements TKN_BRACKET_CLOSE
	  {
		$$ = new ScopedStatements($2);
	  };

return_statement :
	  TKN_RETURN argument_expression TKN_SEMICOLON
	  {
		$$ = new ReturnStatement($2);
	  };

line_statement: 
	  assignment
	| if_statement
	| while_statement
	| break_statement
	| scoped_statement_list
	| valid_expression_as_statement
	| return_statement;

binary_arith_expression:
	  expression TKN_ARITHMETIC_OPERATOR_PLUS expression
	  {
		$$ = new AddOperation($1, $3);
	  }
	| expression TKN_ARITHMETIC_OPERATOR_MINUS expression
	  {
		$$ = new SubtractOperation($1, $3);
	  }
	| expression TKN_ARITHMETIC_OPERATOR_TIMES expression
	  {
		$$ = new MultiplyOperation($1, $3);
	  }
	| expression TKN_ARITHMETIC_OPERATOR_DIVIDE expression
	  {
		$$ = new DivideOperation($1, $3);
	  }
	| expression TKN_ARITHMETIC_OPERATOR_REMAINDER expression
	  {
		$$ = new RemainderOperation($1, $3);
	  }
	| expression TKN_BITWISE_SHIFT_LEFT_OPERATOR expression
	  {
		$$ = new BitwiseShiftLeftOperation($1, $3);
	  }
	| expression TKN_BITWISE_SHIFT_RIGHT_OPERATOR expression
	  {
		$$ = new BitwiseShiftRightOperation($1, $3);
	  }
	| expression TKN_BITWISE_OPERATOR_AND expression
	  {
		$$ = new BitwiseAndOperation($1, $3);
	  }
	| expression TKN_BITWISE_OPERATOR_OR expression
	  {
		$$ = new BitwiseOrOperation($1, $3);
	  }
	| expression TKN_BITWISE_OPERATOR_XOR expression
	  {
		$$ = new BitwiseXorOperation($1, $3);
	  };

unary_arith_expression:
	  single_identifier TKN_ARITHMETIC_OPERATOR_INCREMENT
	  {
		$$ = new PostIncrementOperation($1);
	  }
	| TKN_ARITHMETIC_OPERATOR_INCREMENT single_identifier
	  {
		$$ = new PreIncrementOperation($2);
	  }
	| single_identifier TKN_ARITHMETIC_OPERATOR_DECREMENT
	  {
		$$ = new PostDecrementOperation($1);
	  }
	| TKN_ARITHMETIC_OPERATOR_DECREMENT single_identifier 
	  {
		$$ = new PreDecrementOperation($2);
	  }
	| TKN_BITWISE_OPERATOR_COMPLEMENT expression
	  {
		$$ = new ComplementOperation($2);
	  };

valid_expression_as_statement:
	  unary_arith_expression TKN_SEMICOLON
	  {
		$$ = new ExpressionAsStatement($1);
	  }
	| print_statement TKN_SEMICOLON
	  {
		$$ = new ExpressionAsStatement($1);
	  }
	| function_call TKN_SEMICOLON
	  {
		$$ = new ExpressionAsStatement($1);
	  };

binary_comparison_expression:
	  expression TKN_COMPARISON_OPERATOR_GREATER_THAN expression
	  {
		$$ = new GreaterThanOperation($1, $3);
	  }
	| expression TKN_COMPARISON_OPERATOR_LESS_THAN expression
	  {
		$$ = new LessThanOperation($1, $3);
	  }
	| expression TKN_COMPARISON_OPERATOR_EQUAL_TO expression
	  {
		$$ = new EqualToOperation($1, $3);
	  }
	| expression TKN_COMPARISON_OPERATOR_NOT_EQUAL_TO expression
	  {
		$$ = new NotEqualToOperation($1, $3);
	  }
	| expression TKN_COMPARISON_OPERATOR_GREATER_THAN_OR_EQUAL_TO expression
	  {
		$$ = new GreaterThanOrEqualOperation($1, $3);
	  }
	| expression TKN_COMPARISON_OPERATOR_LESS_THAN_OR_EQUAL_TO expression
	  {
		$$ = new LessThanOrEqualOperation($1, $3);
	  };

logical_expression:
	  expression TKN_LOGICAL_OPERATOR_AND expression
	  {
		$$ = new LogicalAndOperation($1, $3);
	  }
	| expression TKN_LOGICAL_OPERATOR_OR expression
	  {
		$$ = new LogicalOrOperation($1, $3);
	  }
	| TKN_LOGICAL_OPERATOR_NEGATE expression
	  {
		$$ = new NegateOperation($2);
	  };

new_expression:
	  TKN_NEW reference TKN_PAREN_OPEN argument_expression TKN_PAREN_CLOSE
	  {
		$$ = new NewExpression($2->Id(), $4);
		delete $2;
	  };

print_statement:
	  TKN_PRINT TKN_PAREN_OPEN argument_expression TKN_PAREN_CLOSE
	  {
		$$ = new DebugPrintStatement($3);
	  }

function_call:
	  reference TKN_PAREN_OPEN argument_expression TKN_PAREN_CLOSE
	  {
		$$ = new FunctionCall($1->Id(), $3);
		delete $1;
	  };

literal:
	  number_literal
	| bool_literal 
	| char_literal
	| string_literal;

number_literal:
	  TKN_ARITHMETIC_OPERATOR_MINUS TKN_CONSTANT_INT
	  {
		$$ = new IntegerConstant(*$<id>2, true /*negate*/);
		delete $<id>2;
	  }
	| TKN_ARITHMETIC_OPERATOR_MINUS TKN_CONSTANT_FLOAT
	  {
		$$ = new FloatingConstant(*$<id>2,  true /*negate*/);
		delete $<id>2;
	  }
	| TKN_CONSTANT_INT
	  {
		$$ = new IntegerConstant(*$<id>1);
		delete $<id>1;
	  }
	| TKN_CONSTANT_FLOAT
	  {
		$$ = new FloatingConstant(*$<id>1);
		delete $<id>1;
	  };

bool_literal:
	  TKN_CONSTANT_BOOL_TRUE
	  {
		$$ = new BoolConstant(true);
	  }
	| TKN_CONSTANT_BOOL_FALSE
	  {
		$$ = new BoolConstant(false);
	  };

char_literal:
	  TKN_CONSTANT_CHAR
	  {
		$$ = new CharConstant(*$<id>1);
		delete $<id>1;
	  };
	  
string_literal:
	  TKN_CONSTANT_STRING
	  {
		$$ = new StringConstant(*$<id>1);
		delete $<id>1;
	  };

argument_expression:
	expression TKN_COMMA argument_expression
	  {
		$$ = new ExpressionList($1, $3);
	  }
	| expression
	| /*epsilon*/
	  {
		$$ = NULL;
	  };

expression:
	TKN_PAREN_OPEN expression TKN_PAREN_CLOSE
	  { 
		$$ = $2; 
	  }
	| binary_arith_expression
	| unary_arith_expression
	| binary_comparison_expression
	| logical_expression
	| literal
	| reference 
	| new_expression
	| function_call
	| print_statement;