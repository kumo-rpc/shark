/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_THRIFTY_HH_INCLUDED
# define YY_YY_THRIFTY_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "thrifty.yy"

#include <shark/compiler/parse/t_program.h>

#line 53 "thrifty.hh"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    tok_identifier = 258,          /* tok_identifier  */
    tok_literal = 259,             /* tok_literal  */
    tok_doctext = 260,             /* tok_doctext  */
    tok_int_constant = 261,        /* tok_int_constant  */
    tok_dub_constant = 262,        /* tok_dub_constant  */
    tok_include = 263,             /* tok_include  */
    tok_namespace = 264,           /* tok_namespace  */
    tok_cpp_include = 265,         /* tok_cpp_include  */
    tok_cpp_type = 266,            /* tok_cpp_type  */
    tok_xsd_all = 267,             /* tok_xsd_all  */
    tok_xsd_optional = 268,        /* tok_xsd_optional  */
    tok_xsd_nillable = 269,        /* tok_xsd_nillable  */
    tok_xsd_attrs = 270,           /* tok_xsd_attrs  */
    tok_void = 271,                /* tok_void  */
    tok_bool = 272,                /* tok_bool  */
    tok_string = 273,              /* tok_string  */
    tok_binary = 274,              /* tok_binary  */
    tok_uuid = 275,                /* tok_uuid  */
    tok_byte = 276,                /* tok_byte  */
    tok_i8 = 277,                  /* tok_i8  */
    tok_i16 = 278,                 /* tok_i16  */
    tok_i32 = 279,                 /* tok_i32  */
    tok_i64 = 280,                 /* tok_i64  */
    tok_double = 281,              /* tok_double  */
    tok_map = 282,                 /* tok_map  */
    tok_list = 283,                /* tok_list  */
    tok_set = 284,                 /* tok_set  */
    tok_oneway = 285,              /* tok_oneway  */
    tok_async = 286,               /* tok_async  */
    tok_typedef = 287,             /* tok_typedef  */
    tok_struct = 288,              /* tok_struct  */
    tok_xception = 289,            /* tok_xception  */
    tok_throws = 290,              /* tok_throws  */
    tok_extends = 291,             /* tok_extends  */
    tok_service = 292,             /* tok_service  */
    tok_enum = 293,                /* tok_enum  */
    tok_const = 294,               /* tok_const  */
    tok_required = 295,            /* tok_required  */
    tok_optional = 296,            /* tok_optional  */
    tok_union = 297,               /* tok_union  */
    tok_reference = 298            /* tok_reference  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 83 "thrifty.yy"

  char*          id;
  int64_t        iconst;
  double         dconst;
  bool           tbool;
  t_doc*         tdoc;
  t_type*        ttype;
  t_base_type*   tbase;
  t_typedef*     ttypedef;
  t_enum*        tenum;
  t_enum_value*  tenumv;
  t_const*       tconst;
  t_const_value* tconstv;
  t_struct*      tstruct;
  t_service*     tservice;
  t_function*    tfunction;
  t_field*       tfield;
  char*          dtext;
  char*          keyword;
  t_field::e_req ereq;
  t_annotation*  tannot;
  t_field_id     tfieldid;

#line 137 "thrifty.hh"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_THRIFTY_HH_INCLUDED  */
