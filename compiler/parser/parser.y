%{
#include <string>
#include <cstdio>
#include <cstring>

#include "parser/ASTNode.h"
#include "parser/parser.hpp"
#include "common/define_d.h"
#include "utilities/file_util.h"
#include "utilities/log_util.h"
#include "utilities/string_util.h"

extern int yylineno;

extern int yylex();

dap::parser::ProgramNode* program;

struct ParseFlags {
    bool isDefiningNumber = false;
    bool isDefiningType = false;
    bool isUnsignedNum = false;
    bool isDefiningVariable = false;

    BasicType dataType;

} flags;

void clearParseFlags() {
    flags.isDefiningNumber = false;
    flags.isDefiningType = false;
    flags.isUnsignedNum = false;
    flags.isDefiningVariable = false;
}

void defineUnsignedNum() {
    flags.isUnsignedNum = true;
}

void defineType() {
    flags.isDefiningType = true;
}

namespace dap::parser {
extern std::string currentParsingFile;
}

void yyerror(const char *s) {
    fprintf(stderr, "error: %s\n at %s:%d \n", s, dap::parser::currentParsingFile.c_str(), yylineno);
}
void parserLog(const char *msg) {
#ifdef D_DEBUG
    dap::util::logInfo(msg, nullptr, dap::parser::currentParsingFile, yylineno);
#endif
}

void parserLog(std::string msg) {
#ifdef D_DEBUG
    dap::util::logInfo(msg, nullptr, dap::parser::currentParsingFile, yylineno);
#endif
}

std::string tokenToString(int token) {
    switch (token) {
        case PACKAGE:
            return "PACKAGE";
        case IMPORT:
            return "IMPORT";
        case FUN:
            return "FUN";
        case VOID:
            return "VOID";
        case FOR:
            return "FOR";
        case IF:
            return "IF";
        case ELSE:
            return "ELSE";
        case MATCH:
            return "MATCH";
        case STRUCT:
            return "STRUCT";
        case TRAIT:
            return "TRAIT";
        case TYPEDEF:
            return "TYPEDEF";
        case IMT:
            return "IMT";
        case VAR:
            return "VAR";
        case INSTANCEOF:
            return "INSTANCEOF";
        case RETURN:
            return "RETURN";
        case CONST:
            return "CONST";
        case EXTERN:
            return "EXTERN";
        case INT:
            return "INT";
        case BYTE:
            return "BYTE";
        case SHORT:
            return "SHORT";
        case LONG:
            return "LONG";
        case FLOAT:
            return "FLOAT";
        case DOUBLE:
            return "DOUBLE";
        case BOOL:
            return "BOOL";
        case UINT:
            return "UINT";
        case USHORT:
            return "USHORT";
        case ULONG:
            return "ULONG";
        case LLONG:
            return "LLONG";
        case ULLONG:
            return "ULLONG";
        case IDENTIFIER:
            return "IDENTIFIER";
        case INTEGER:
            return "INTEGER";
        case BINARY_LITERAL:
            return "BINARY_LITERAL";
        case OCTAL_LITERAL:
            return "OCTAL_LITERAL";
        case HEXADECIMAL_LITERAL:
            return "HEXADECIMAL_LITERAL";
        case STRING_LITERAL:
            return "STRING_LITERAL";
        case CHAR_LITERAL:
            return "CHAR_LITERAL";
        case FLOAT_LITERAL:
            return "FLOAT_LITERAL";
        case TRUE:
            return "TRUE";
        case FALSE:
            return "FALSE";
        case PLUS:
            return "PLUS";
        case MINUS:
            return "MINUS";
        case MUL:
            return "MUL";
        case DIV:
            return "DIV";
        case MOD:
            return "MOD";
        case BIT_AND:
            return "BIT_AND";
        case BIT_OR:
            return "BIT_OR";
        case BIT_XOR:
            return "BIT_XOR";
        case BIT_NOT:
            return "BIT_NOT";
        case SHIFT_LEFT:
            return "SHIFT_LEFT";
        case SHIFT_RIGHT:
            return "SHIFT_RIGHT";
        case LOGIC_SHIFT_RIGHT:
            return "LOGIC_SHIFT_RIGHT";
        case ASSIGN:
            return "ASSIGN";
        case ADD_ASSIGN:
            return "ADD_ASSIGN";
        case MUL_ASSIGN:
            return "MUL_ASSIGN";
        case DIV_ASSIGN:
            return "DIV_ASSIGN";
        case MINUS_ASSIGN:
            return "MINUS_ASSIGN";
        case MOD_ASSIGN:
            return "MOD_ASSIGN";
        case AND_ASSIGN:
            return "AND_ASSIGN";
        case OR_ASSIGN:
            return "OR_ASSIGN";
        case BIT_AND_ASSIGN:
            return "BIT_AND_ASSIGN";
        case BIT_OR_ASSIGN:
            return "BIT_OR_ASSIGN";
        case BIT_XOR_ASSIGN:
            return "BIT_XOR_ASSIGN";
        case SHIFT_LEFT_ASSIGN:
            return "SHIFT_LEFT_ASSIGN";
        case SHIFT_RIGHT_ASSIGN:
            return "SHIFT_RIGHT_ASSIGN";
        case LOGIC_SHIFT_RIGHT_ASSIGN:
            return "LOGIC_SHIFT_RIGHT_ASSIGN";
        case INCREMENT:
            return "INCREMENT";
        case DECREMENT:
            return "DECREMENT";
        case LESS_THAN:
            return "LESS_THAN";
        case GREATER_THAN:
            return "GREATER_THAN";
        case LESS_THAN_EQUAL:
            return "LESS_THAN_EQUAL";
        case GREATER_THAN_EQUAL:
            return "GREATER_THAN_EQUAL";
        case EQUAL:
            return "EQUAL";
        case NOT_EQUAL:
            return "NOT_EQUAL";
        case AND:
            return "AND";
        case OR:
            return "OR";
        case XOR:
            return "XOR";
        case COMMA:
            return "COMMA";
        case SEMICOLON:
            return "SEMICOLON";
        case COLON:
            return "COLON";
        case LEFT_BRACE:
            return "LEFT_BRACE";
        case RIGHT_BRACE:
            return "RIGHT_BRACE";
        case LEFT_PAREN:
            return "LEFT_PAREN";
        case RIGHT_PAREN:
            return "RIGHT_PAREN";
        case LEFT_BRACKET:
            return "LEFT_BRACKET";
        case RIGHT_BRACKET:
            return "RIGHT_BRACKET";
        case DOT:
            return "DOT";
        case ELLIPSIS:
            return "ELLIPSIS";
        case QUESTION:
            return "QUESTION";
        case BANG:
            return "BANG";
        default:
            return "UNKNOWN";
    }
}

%}

%union {
#include <vector>
#include "parser/ASTNode.h"

    // dap::parser::ASTNode *node;
    // dap::parser::BlockStmt *block;
    dap::parser::Expression *expr;
    dap::parser::Statement *stmt;
    dap::parser::QualifiedNameNode *ident;
    dap::parser::TypeNode *typeNode;
    std::vector<dap::parser::TypeNode*> *typeNodeVec;
    dap::parser::IntegerNode *intExpr;
    dap::parser::StringNode *strExpr;
    dap::parser::FloatNode *floatExpr;
    std::vector<dap::parser::Expression*> *exprVec;
    std::vector<dap::parser::Statement*> *stmtVec;
    std::vector<dap::parser::VariableDeclarationNode*> *varDeclVec;
    std::string *str;
    char charVal;
    int IntegerNode;
    double floatVal;
    bool boolval;
    int token;
}

// Define the tokens
%token PACKAGE IMPORT FUN VOID FOR IF ELSE MATCH STRUCT TRAIT TYPEDEF IMT VAR INSTANCEOF RETURN CONST EXTERN
%token INT BYTE SHORT LONG FLOAT DOUBLE BOOL UINT USHORT ULONG LLONG ULLONG
%token IDENTIFIER INTEGER BINARY_LITERAL OCTAL_LITERAL HEXADECIMAL_LITERAL
%token STRING_LITERAL CHAR_LITERAL FLOAT_LITERAL TRUE FALSE 
%token PLUS MINUS MUL DIV MOD BIT_AND BIT_OR BIT_XOR BIT_NOT SHIFT_LEFT SHIFT_RIGHT LOGIC_SHIFT_RIGHT
%token ASSIGN ADD_ASSIGN MUL_ASSIGN DIV_ASSIGN MINUS_ASSIGN MOD_ASSIGN
%token AND_ASSIGN OR_ASSIGN
%token BIT_AND_ASSIGN BIT_OR_ASSIGN BIT_XOR_ASSIGN SHIFT_LEFT_ASSIGN SHIFT_RIGHT_ASSIGN LOGIC_SHIFT_RIGHT_ASSIGN
%token INCREMENT DECREMENT LESS_THAN GREATER_THAN LESS_THAN_EQUAL GREATER_THAN_EQUAL EQUAL NOT_EQUAL
%token AND OR XOR
%token COMMA SEMICOLON COLON LEFT_BRACE RIGHT_BRACE LEFT_PAREN RIGHT_PAREN LEFT_BRACKET RIGHT_BRACKET
%token DOT ELLIPSIS QUESTION BANG  

%type <str> IDENTIFIER INTEGER BINARY_LITERAL OCTAL_LITERAL HEXADECIMAL_LITERAL FLOAT_LITERAL STRING_LITERAL
%type <ident> identifier
%type <expr> expression functionCall bool_ binaryExpression unaryExpression arrayExpression truncExpression
%type <stmt> importStmt packageDecl statement functionDeclaration variableDecl constantDecl structDecl returnStmt ifStatement forStatement
%type <stmtVec> statements importStmts
%type <exprVec> expressions 
%type <typeNode> type
%type <boolval> mutableModifier nullableModifier TRUE FALSE 
%type <token> binaryOperator unaryOperator
%type <intExpr> integer
%type <floatExpr> float_
%type <strExpr> string_ CHAR_LITERAL
%type <varDeclVec> structFields functionParameters

%start program
%%
program:
    packageDecl importStmts statements {
        // Create a new program node
        program->statements = $3;
        // Log message when parsing a program node
        parserLog("Parsed program node");
    };

variableDecl:
    mutableModifier type identifier nullableModifier ASSIGN expression {
        $$ = new dap::parser::VariableDeclarationNode($2, $6, $3, $4, $1);
        // Log message when parsing a variable declaration node
        $$->lineNum = yylineno;
        parserLog("Parsed variable declaration node");
    }
    | mutableModifier identifier nullableModifier ASSIGN expression {
        $$ = new dap::parser::VariableDeclarationNode(nullptr, $5, $2, $3, $1);
        // Log message when parsing a variable declaration node without explicit type
        $$->lineNum = yylineno;
        parserLog("Parsed variable declaration node without explicit type");
    }
    | mutableModifier type identifier nullableModifier {
        $$ = new dap::parser::VariableDeclarationNode($2, nullptr, $3, $4, $1);
        // Log message when parsing a variable declaration node
        $$->lineNum = yylineno;
        parserLog("Parsed variable declaration node");
    }
    | mutableModifier identifier nullableModifier {
        $$ = new dap::parser::VariableDeclarationNode(nullptr, nullptr, $2, $3, $1);
        // Log message when parsing a variable declaration node without explicit type
        $$->lineNum = yylineno;
        parserLog("Parsed variable declaration node without explicit type");
    }
    ;

constantDecl:
    CONST type identifier ASSIGN expression {
        $$ = new dap::parser::ConstantDeclarationNode($2, $3, $5);
        // Log message when parsing a constant declaration node
        $$->lineNum = yylineno;
        parserLog("Parsed constant declaration node");
    } 
    | CONST identifier ASSIGN expression {
        $$ = new dap::parser::ConstantDeclarationNode(nullptr, $2, $4);
        // Log message when parsing a constant declaration node
        $$->lineNum = yylineno;
        parserLog("Parsed constant declaration node");
    }
    ;

expression:
    integer {
        $$ = $1;
        // Log message when parsing an IntegerNode expression node
        $$->lineNum = yylineno;
        parserLog("Parsed IntegerNode expression node");
    }
    | identifier {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed identifier expression node: [" + $1->getName() + "]");
    }
    | float_ {
        $$ = $1;
        // Log message when parsing a FloatNode expression node
        $$->lineNum = yylineno;
        parserLog("Parsed FloatNode expression node");
    }
    | string_ {
        $$ = $1;
        // Log message when parsing a StringNode expression node
        $$->lineNum = yylineno;
        parserLog("Parsed StringNode expression node");
    }
    | bool_ {
        $$ = $1;
        $$->lineNum = yylineno;
        parserLog("Parsed Boolean expression node"); 
    }
    | functionCall {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed function call expression node: [" + dynamic_cast<dap::parser::FunctionCallExpressionNode*>($1)
                                                        ->name->getName() + "]");
    }
    | binaryExpression {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed binary expression node");
    }
    | unaryExpression {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed unary expression node");
    }
    | arrayExpression {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed array expression node");
    }
    | truncExpression {
        $$ = $1;

        $$->lineNum = yylineno;
        parserLog("Parsed trunc expression node");
    }
    | LEFT_PAREN expression RIGHT_PAREN {
        $$ = $2;
        $$->lineNum = yylineno;
        parserLog("Parsed parenthesized expression node");
    };

functionDeclaration:
    FUN identifier LEFT_PAREN RIGHT_PAREN LEFT_BRACE statements RIGHT_BRACE {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }
        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        nullptr,
                                                        nullptr,
                                                        $6);
        // Log message when parsing a function declaration node without parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node without parameters");
    }
    | FUN identifier LEFT_PAREN RIGHT_PAREN type LEFT_BRACE statements RIGHT_BRACE {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }
        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        nullptr,
                                                        $5,
                                                        $7);
        // Log message when parsing a function declaration node without parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node without parameters");
    }
    | FUN identifier LEFT_PAREN functionParameters RIGHT_PAREN LEFT_BRACE statements RIGHT_BRACE {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }

        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        $4,
                                                        nullptr,
                                                        $7);
        // Log message when parsing a function declaration node with parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node with parameters");

    }
    | FUN identifier LEFT_PAREN functionParameters RIGHT_PAREN type LEFT_BRACE statements RIGHT_BRACE {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }

        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        $4,
                                                        $6,
                                                        $8);
        // Log message when parsing a function declaration node with parameters and return type
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node with parameters and return type");
    }
    | FUN identifier LEFT_PAREN RIGHT_PAREN SEMICOLON {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }
        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr);
        // Log message when parsing a function declaration node without parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node without parameters");
    }
    | FUN identifier LEFT_PAREN RIGHT_PAREN type SEMICOLON {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }
        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr);
        // Log message when parsing a function declaration node without parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node without parameters");
    }
    | FUN identifier LEFT_PAREN functionParameters RIGHT_PAREN SEMICOLON {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }

        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        $4,
                                                        nullptr,
                                                        nullptr);
        // Log message when parsing a function declaration node with parameters
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node with parameters");

    }
    | FUN identifier LEFT_PAREN functionParameters RIGHT_PAREN type SEMICOLON {
        if ($2->name_parts->size()!= 1) {
            // Call the error function if the identifier is invalid
            yyerror("Invalid identifier");
        }

        $$ = new dap::parser::FunctionDeclarationNode($2->name_parts->at(0),
                                                        $4,
                                                        $6,
                                                        nullptr);
        // Log message when parsing a function declaration node with parameters and return type
        $$->lineNum = yylineno;
        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed function declaration node with parameters and return type");
    };


functionParameters:
    /* empty */ {
        $$ = new std::vector<dap::parser::VariableDeclarationNode*>();
        // Log message when parsing a variable declaration node without explicit type

        parserLog("Parsed variable declaration node without explicit type");
    }
    | variableDecl  {
        $$ = new std::vector<dap::parser::VariableDeclarationNode*>();
        $$->push_back(dynamic_cast<dap::parser::VariableDeclarationNode*>($1));
        // Log message when parsing a variable declaration node

        parserLog("Parsed variable declaration node");
    }
    | functionParameters COMMA variableDecl  {
        $$ = $1;
        $1->push_back(dynamic_cast<dap::parser::VariableDeclarationNode*>($3));
        // Log message when parsing a variable declaration node

        parserLog("Parsed variable declaration node");
    };

ifStatement: 
    IF expression LEFT_BRACE statements RIGHT_BRACE {
        $$ = new dap::parser::IfStatementNode($2, $4);
        // Log message when parsing an if statement node
        $$->lineNum = yylineno;
        parserLog("Parsed if statement node");
    }
    | IF expression LEFT_BRACE statements RIGHT_BRACE ELSE LEFT_BRACE statements RIGHT_BRACE {
        $$ = new dap::parser::IfStatementNode($2, $4, $8);
        // Log message when parsing an if statement node
        $$->lineNum = yylineno;
        parserLog("Parsed if statement node");
    }
    | IF expression LEFT_BRACE statements RIGHT_BRACE ELSE ifStatement {
        $$ = new dap::parser::IfStatementNode($2, $4, nullptr, $7);
        // Log message when parsing an if statement node
        $$->lineNum = yylineno;
        parserLog("Parsed if statement node");
    }
    ;

forStatement:
    FOR variableDecl SEMICOLON expression SEMICOLON expression LEFT_BRACE statements RIGHT_BRACE {
        $$ = new dap::parser::ForStatementNode(dynamic_cast<dap::parser::VariableDeclarationNode*>($2), $4, $6, $8);
        // Log message when parsing an if statement node
        $$->lineNum = yylineno;
        parserLog("Parsed for statement node");
    };
    
nullableModifier:
    /* empty */ {
        $$ = false;
        // Log message when parsing a variable declaration node without explicit type

        parserLog("Parsed variable declaration node without explicit nullablity");
    }
    | QUESTION {
        $$ = true;
        // Log message when parsing a variable declaration node

        parserLog("Parsed variable declaration node");
        $$ = true;
    }
    | BANG {
        $$ = false;
        // Log message when parsing a variable declaration node

        parserLog("Parsed variable declaration node");
    };

mutableModifier:
    /* empty */ {
        $$ = false;

        parserLog("Parsed variable declaration node without explicit mutability");
    }
    | IMT {
        $$ = false;

        parserLog("Parsed variable declaration node");
    }
    | VAR {
        $$ = true;

        parserLog("Parsed variable declaration node");
    };


identifier:
    IDENTIFIER {
        $$ = new dap::parser::QualifiedNameNode(*$1);
        delete $1;
        // Log message when parsing an identifier node
        $$->lineNum = yylineno;
        parserLog("Parsed identifier node");
    }
    | identifier DOT IDENTIFIER {
        $1->name_parts->push_back(*$3);
        $$ = $1;
        delete $3;
        // Log message when parsing a qualified identifier node
        $$->lineNum = yylineno;
        parserLog("Parsed qualified identifier node");
    };

integer:
    INTEGER {

        if (flags.dataType == BasicType::UNKNOWN) {
            flags.dataType = BasicType::INT;
        }

        $$ = new dap::parser::IntegerNode($1, flags.dataType);
        // Log message when parsing an IntegerNode node
        $$->lineNum = yylineno;

        parserLog("Parsed IntegerNode node: integer[" + $$->getVal() + "]");
    };

float_:
    FLOAT_LITERAL {
        $$ = new dap::parser::FloatNode(atof($1->c_str()));
        // Log message when parsing a float node
        $$->lineNum = yylineno;
        parserLog("Parsed float node");
    };

string_:
    STRING_LITERAL {

        $$ = new dap::parser::StringNode(*dap::util::getPureStr($1));
        // Log message when parsing a string node
        $$->lineNum = yylineno;
        parserLog("Parsed string node");
    };

bool_:
    TRUE {
        $$ = new dap::parser::BoolNode(true);
        // Log message when parsing a boolean node
        $$->lineNum = yylineno;
        parserLog("Parsed boolean node: [ TRUE ]");
    }
    | FALSE {
        $$ = new dap::parser::BoolNode(false);
        // Log message when parsing a boolean node
        $$->lineNum = yylineno;
        parserLog("Parsed boolean node: [ FALSE ]");
    };

type:
    identifier {
        $$ = new dap::parser::TypeNode($1);
        // Log message when parsing a type node
        $$->lineNum = yylineno;
        parserLog("Parsed type node");
    }
    | type MUL {
        $$ = $1;
        $$->isPointer = true;
        // Log message when parsing a pointer type node
        $$->lineNum = yylineno;
        parserLog("Parsed pointer type node");
    }
    | type LEFT_BRACKET RIGHT_BRACKET {
        $$ = $1;
        $$->isArray = true;
        // Log message when parsing an array type node without size
        $$->lineNum = yylineno;
        parserLog("Parsed array type node without size");
    }
    | type LEFT_BRACKET integer RIGHT_BRACKET {
        $$ = $1;
        $$->isArray = true;
        $$->arraySize = std::stoi($3->getVal());
        // Log message when parsing an array type node with size
        $$->lineNum = yylineno;
        parserLog("Parsed array type node with size");
    }
    | INT {
        flags.dataType = BasicType::INT;
        $$ = new dap::parser::TypeNode(BasicType::INT);
        // Log message when parsing an int type node
        $$->lineNum = yylineno;
        parserLog("Parsed int type node");
    }
    | BYTE {
        flags.dataType = BasicType::BYTE;
        $$ = new dap::parser::TypeNode(BasicType::BYTE);
        // Log message when parsing a byte type node
        $$->lineNum = yylineno;
        parserLog("Parsed byte type node");
    }
    | SHORT {
        flags.dataType = BasicType::SHORT;
        $$ = new dap::parser::TypeNode(BasicType::SHORT);
        // Log message when parsing a short type node
        $$->lineNum = yylineno;
        parserLog("Parsed short type node");
    }
    | LONG {
        flags.dataType = BasicType::LONG;
        $$ = new dap::parser::TypeNode(BasicType::LONG);
        // Log message when parsing a long type node
        $$->lineNum = yylineno;
        parserLog("Parsed long type node");
    }
    | FLOAT {
        flags.dataType = BasicType::FLOAT;
        $$ = new dap::parser::TypeNode(BasicType::FLOAT);
        // Log message when parsing a float type node
        $$->lineNum = yylineno;
        parserLog("Parsed float type node");
    }
    | DOUBLE {
        flags.dataType = BasicType::DOUBLE;
        $$ = new dap::parser::TypeNode(BasicType::DOUBLE);
        // Log message when parsing a double type node
        $$->lineNum = yylineno;
        parserLog("Parsed double type node");
    }
    | BOOL {
        flags.dataType = BasicType::BOOL;
        $$ = new dap::parser::TypeNode(BasicType::BOOL);
        // Log message when parsing a bool type node
        $$->lineNum = yylineno;
        parserLog("Parsed bool type node");
    }
    | UINT {
        flags.dataType = BasicType::UINT;
        $$ = new dap::parser::TypeNode(BasicType::UINT);
        // Log message when parsing a uint type node
        $$->lineNum = yylineno;
        parserLog("Parsed uint type node");
    }
    | USHORT {
        flags.dataType = BasicType::USHORT;
        $$ = new dap::parser::TypeNode(BasicType::USHORT);
        // Log message when parsing a ushort type node
        $$->lineNum = yylineno;
        parserLog("Parsed ushort type node");
    }
    | ULONG {
        flags.dataType = BasicType::ULONG;
        $$ = new dap::parser::TypeNode(BasicType::ULONG);
        // Log message when parsing a ulong type node
        $$->lineNum = yylineno;
        parserLog("Parsed ulong type node");
    }
    | LLONG {
        flags.dataType = BasicType::LLONG;
        $$ = new dap::parser::TypeNode(BasicType::LLONG);
        // Log message when parsing a lllong type node
        $$->lineNum = yylineno;
        parserLog("Parsed lllong type node");
    }
    | ULLONG {
        flags.dataType = BasicType::ULLONG;
        $$ = new dap::parser::TypeNode(BasicType::ULLONG);
        // Log message when parsing a ullong type node
        $$->lineNum = yylineno;
        parserLog("Parsed ullong type node");
    }
    | VOID {
        flags.dataType = BasicType::VOID;
        $$ = new dap::parser::TypeNode(BasicType::VOID);
        // Log message when parsing a void type node
        $$->lineNum = yylineno;
        parserLog("Parsed void type node");
    };

packageDecl:
    /* empty */ {

    } |
    PACKAGE identifier SEMICOLON {
        program->packageName = $2;
        // Log message when parsing a package declaration node
        parserLog("Parsed package declaration node: package [" + $2->getName() + "]");
    };

importStmt:
    IMPORT identifier SEMICOLON {
        auto info = new dap::parser::ProgramNode::importedPackageInfo($2, false);
        program->importedPackages->push_back(info);
        // Log message when parsing an import statement node
        parserLog("Parsed import statement node: import [" + $2->getName() + "]");
    }
    | IMPORT identifier DOT MUL SEMICOLON {
        auto info = new dap::parser::ProgramNode::importedPackageInfo($2, true);
        program->importedPackages->push_back(info);
        // Log message when parsing an import wildcard statement node
        parserLog("Parsed import wildcard statement node: import [" + $2->getName() + "] [all]");
    };

importStmts:
    /* empty */ {

    }
    | importStmts importStmt {

    };

statement:
    functionDeclaration {
        $$ = $1;
        // Log message when parsing a function declaration statement node
        $$->lineNum = yylineno;
        parserLog("Parsed function declaration statement node: [" + (dynamic_cast<dap::parser::FunctionDeclarationNode*>($1))->name + "]");
    }
    | variableDecl SEMICOLON {
        $$ = $1;
        // Log message when parsing a variable declaration statement node
        $$->lineNum = yylineno;

        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed variable declaration statement node: [" + (dynamic_cast<dap::parser::VariableDeclarationNode*>($1))->variableName->getName() + "]");
    }
    | constantDecl SEMICOLON { 
        $$ = $1;
        // Log message when parsing a constant declaration statement node
        $$->lineNum = yylineno;

        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed constant declaration statement node: [" + (dynamic_cast<dap::parser::ConstantDeclarationNode*>($1))->name->getName() + "]");
    } 
    | structDecl {
        $$ = $1;
        // Log message when parsing a struct declaration statement node
        $$->lineNum = yylineno;

        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed struct declaration statement node: [" + (dynamic_cast<dap::parser::StructDeclarationNode*>($1))->name->getName() + "]");
    }
    | returnStmt {
        $$ = $1;
        // Log message when parsing a return statement node
        $$->lineNum = yylineno;

        flags.dataType = BasicType::UNKNOWN;

        parserLog("Parsed return statement node");
    }
    | expression SEMICOLON {
        $$ = new dap::parser::Statement();
        $$->value = $1;
        // Log message when parsing a function call statement node
        $$->lineNum = yylineno;
        parserLog("Parsed function call statement node");
    }
    | ifStatement {
        $$ = $1;

    }
    | forStatement {
        $$ = $1;
    };


statements:
    /* empty */ {
        $$ = new std::vector<dap::parser::Statement*>();
        // Log message when starting to parse a list of statements

        flags.dataType = BasicType::UNKNOWN;
        parserLog("Started parsing statements list");
    }
    | statements statement {
        $$->push_back($2);
        // Log message when adding a statement to the statements list

        flags.dataType = BasicType::UNKNOWN;
        parserLog("Added statement to statements list");
    };

expressions:
    /* empty */ {
        $$ = new std::vector<dap::parser::Expression*>();
        // Log message when starting to parse a list of expressions

        parserLog("Started parsing expressions list");
    }
    | expression {
        $$ = new std::vector<dap::parser::Expression*>();
        $$->push_back($1);

        parserLog("Started parsing expressions list");
    }
    | expressions COMMA expression {
        $$->push_back($3);
        // Log message when adding an expression to the expressions list

        parserLog("Added expression to expressions list");
    };

structDecl: 
    STRUCT identifier LEFT_BRACE structFields RIGHT_BRACE SEMICOLON {
        $$ = new dap::parser::StructDeclarationNode($2, $4);
        $$->lineNum = yylineno;
        parserLog("Parsed struct declaration node: [" + $2->getName() + "]");
    };


structFields:
    /* empty */ {
        $$ = new std::vector<dap::parser::VariableDeclarationNode*>();
        // Log message when starting to parse a list of struct fields

        parserLog("Started parsing struct fields list");
    }
    | structFields variableDecl SEMICOLON {
        $$->push_back(dynamic_cast<dap::parser::VariableDeclarationNode*>($2));
        // Log message when adding a struct field to the struct fields list

        parserLog("Added struct field to struct fields list");
    };

returnStmt:
    RETURN expression SEMICOLON {
        $$ = new dap::parser::ReturnStatementNode($2);
        // Log message when parsing a return statement node
        $$->lineNum = yylineno;
        parserLog("Parsed return statement node");
    }
    | RETURN SEMICOLON {
        $$ = new dap::parser::ReturnStatementNode(nullptr);
        // Log message when parsing a return statement node
        $$->lineNum = yylineno;
        parserLog("Parsed return statement node [no value returned]");
    };


functionCall:
    identifier LEFT_PAREN expressions RIGHT_PAREN {
        $$ = new dap::parser::FunctionCallExpressionNode($1, $3);
        // Log message when parsing a function call statement node
        $$->lineNum = yylineno;
        parserLog("Parsed function call experssion node: [" + $1->getName() + "]");
    };

binaryExpression:
    expression binaryOperator expression {
        $$ = new dap::parser::BinaryExpressionNode($1, $2, $3);
        // Log message when parsing a binary expression node
        $$->lineNum = yylineno;
        parserLog("Parsed binary expression node: [" + tokenToString($2) + "]");
    };

arrayExpression:
    identifier LEFT_BRACKET expression RIGHT_BRACKET {
        $$ = new dap::parser::ArrayExpressionNode($1, $3);
        // Log message when parsing an array expression node
        $$->lineNum = yylineno;
        parserLog("Parsed array expression node: [" + $1->getName() + "]");
    };

truncExpression:
    LEFT_PAREN type RIGHT_PAREN LEFT_PAREN expression RIGHT_PAREN {
        $$ = new dap::parser::TruncExpressionNode($2, $5);
        // Log message when parsing a trunc expression node
        $$->lineNum = yylineno;
        parserLog("Parsed trunc expression node: [" + $2->getName() + "]");
    };

binaryOperator:
    ASSIGN {
        $$ = ASSIGN;
    }
    | PLUS {
        $$ = PLUS;
    }
    | MINUS {
        $$ = MINUS;
    }
    | MUL {
        $$ = MUL;
    }
    | DIV {
        $$ = DIV;
    }
    | MOD {
        $$ = MOD;
    }
    | EQUAL {
        $$ = EQUAL;
    }
    | NOT_EQUAL {
        $$ = NOT_EQUAL;
    }
    | ADD_ASSIGN {
        $$ = ADD_ASSIGN;
    }
    | MINUS_ASSIGN {
        $$ = MINUS_ASSIGN;
    }
    | MUL_ASSIGN {
        $$ = MUL_ASSIGN;
    }
    | DIV_ASSIGN {
        $$ = DIV_ASSIGN;
    }
    | MOD_ASSIGN {
        $$ = MOD_ASSIGN;
    }
    | AND {
        $$ = AND;
    }
    | OR {
        $$ = OR;
    }
    | XOR {
        $$ = XOR;
    }
    | AND_ASSIGN {
        $$ = AND_ASSIGN;
    }
    | OR_ASSIGN {
        $$ = OR_ASSIGN;
    }
    | GREATER_THAN {
        $$ = GREATER_THAN;
    }
    | LESS_THAN {
        $$ = LESS_THAN;
    }
    | GREATER_THAN_EQUAL {
        $$ = GREATER_THAN_EQUAL;
    }
    | LESS_THAN_EQUAL {
        $$ = LESS_THAN_EQUAL;
    };


unaryExpression:
    unaryOperator expression {
        $$ = new dap::parser::UnaryExpressionNode($1, $2);
        // Log message when parsing a unary expression node
        $$->lineNum = yylineno;
        parserLog("Parsed unary expression node: [" + tokenToString($1) + "]");
    };

unaryOperator:
    PLUS {
        $$ = PLUS;
    }
    | MINUS {
        $$ = MINUS;
    }
    | BIT_AND {
        $$ = BIT_AND;
    }
    | BANG {
        $$ = BANG;
    }
    | MUL {
        $$ = MUL;
    }
    | INCREMENT {
        $$ = INCREMENT;
    }
    | DECREMENT {
        $$ = DECREMENT;
    };
%%

