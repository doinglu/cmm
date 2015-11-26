
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
     L_DEFINED_NAME = 262,
     L_IDENTIFIER = 263,
     L_EFUN = 264,
     L_FUN = 265,
     L_OSFUN = 266,
     L_OSVAR = 267,
     L_LABEL = 268,
     L_NIL = 269,
     L_IS_REF = 270,
     L_ADD_COMPONENT = 271,
     L_USING = 272,
     L_USING_STATEMENT = 273,
     L_INC = 274,
     L_DEC = 275,
     L_ASSIGN = 276,
     L_LAND = 277,
     L_LOR = 278,
     L_LSH = 279,
     L_RSH = 280,
     L_ORDER = 281,
     L_NOT = 282,
     L_BUILD = 283,
     L_IF = 284,
     L_ELSE = 285,
     L_SWITCH = 286,
     L_CASE = 287,
     L_DEFAULT = 288,
     L_RANGE = 289,
     L_DOT_DOT_DOT = 290,
     L_WHILE = 291,
     L_DO = 292,
     L_FOR = 293,
     L_LOOP = 294,
     L_UPTO = 295,
     L_DOWNTO = 296,
     L_EACH = 297,
     L_IN = 298,
     L_BREAK = 299,
     L_CONTINUE = 300,
     L_ARROW = 301,
     L_EXPAND_ARROW = 302,
     L_COLON_COLON = 303,
     L_ARRAY_OPEN = 304,
     L_MAPPING_OPEN = 305,
     L_FUNCTION_OPEN = 306,
     L_GOTO = 307,
     L_CALL = 308,
     L_RETURN = 309,
     L_TRACE = 310,
     L_SHOUT = 311,
     L_GLOBAL = 312,
     L_TRY = 313,
     L_CATCH = 314,
     L_PRIVATE = 315,
     L_PUBLIC = 316,
     L_OVERRIDE = 317,
     L_NOMASK = 318,
     L_VARARGS = 319,
     L_STATIC = 320,
     L_NOSAVE = 321,
     L_CONST = 322,
     L_FILE = 323,
     LOWER_THAN_ELSE = 324,
     L_NE = 325,
     L_EQ = 326
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 122 "z:\\doing\\Project\\mts\\mts\\cmm_grammar.yyy"

    Integer             number;
    Real                real;
    char*               string;
    ValueType           basic_type;
    AstVarType       type;
    AstVariableInfo        variable;
    SyntaxVariable*		    declare_var;
    AstFunction*     fun;
    AstFunctionArg*  argument;
    AstFunctionArgs* arg_list;
    AstElement*      element;
    AstCase*         switch_case;
    AstFunctionDesc        function_desc;
    AstVariableDesc             var_desc;
    AstLValueInfo*   lv_info;
    AstExpression*   expression;
    AstExpressions*  expressions;
    SyntaxSourceNode*   source_node;
    AstDeclare*      declares;
    SyntaxMappingPair*  mapping_pair;
    Vector_T            array;
    SyntaxContextState  context_state;



/* Line 1676 of yacc.c  */
#line 150 "z:\\doing\\Project\\mts\\mts\\cmm_grammar.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


