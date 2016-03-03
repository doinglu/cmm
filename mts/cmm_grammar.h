
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     L_STRING = 258,
     L_NUMBER = 259,
     L_REAL = 260,
     L_BASIC_TYPE = 261,
     L_IDENTIFIER = 262,
     L_LABEL = 263,
     L_NIL = 264,
     L_IS_REF = 265,
     L_ADD_COMPONENT = 266,
     L_INC = 267,
     L_DEC = 268,
     L_ASSIGN = 269,
     L_LAND = 270,
     L_LOR = 271,
     L_LSH = 272,
     L_RSH = 273,
     L_ORDER = 274,
     L_NOT = 275,
     L_REV = 276,
     L_ADD = 277,
     L_SUB = 278,
     L_MUL = 279,
     L_DIV = 280,
     L_MOD = 281,
     L_OR = 282,
     L_AND = 283,
     L_XOR = 284,
     L_LT = 285,
     L_BUILD = 286,
     L_IF = 287,
     L_ELSE = 288,
     L_SWITCH = 289,
     L_CASE = 290,
     L_DEFAULT = 291,
     L_RANGE = 292,
     L_DOT_DOT_DOT = 293,
     L_WHILE = 294,
     L_DO = 295,
     L_FOR = 296,
     L_LOOP = 297,
     L_UPTO = 298,
     L_DOWNTO = 299,
     L_EACH = 300,
     L_IN = 301,
     L_BREAK = 302,
     L_CONTINUE = 303,
     L_ARROW = 304,
     L_EXPAND_ARROW = 305,
     L_COLON_COLON = 306,
     L_ARRAY_OPEN = 307,
     L_MAPPING_OPEN = 308,
     L_FUNCTION_OPEN = 309,
     L_GOTO = 310,
     L_CALL = 311,
     L_RETURN = 312,
     L_TRACE = 313,
     L_SHOUT = 314,
     L_GLOBAL = 315,
     L_TRY = 316,
     L_CATCH = 317,
     L_PRIVATE = 318,
     L_PUBLIC = 319,
     L_OVERRIDE = 320,
     L_NOMASK = 321,
     L_STATIC = 322,
     L_NOSAVE = 323,
     L_CONST = 324,
     LOWER_THAN_ELSE = 325,
     L_NE = 326,
     L_EQ = 327
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 116 "Z:\\doing\\Project\\mts\\mts\\cmm_grammar.yyy"

    Integer             number;
    Real                real;
    const char*         string;

    AstNode*            node;
    AstCase*            cases;
    AstDeclaration*     declares;
    AstExpr*            expression;
    AstExpr*            expr_list;
    AstFunction*        function;
    AstFunctionArg*     argument;
    AstFunctionArgsEx   arguments_ex;
    AstPrototype*       prototype;

    ValueType           basic_var_type;
    AstVarAttrib        var_attrib;
    AstVarType          var_type;
    AstFunctionAttrib   fun_attrib;

    int                 int_val;
    Op                  op;



/* Line 1676 of yacc.c  */
#line 150 "Z:\\doing\\Project\\mts\\mts\\cmm_grammar.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


