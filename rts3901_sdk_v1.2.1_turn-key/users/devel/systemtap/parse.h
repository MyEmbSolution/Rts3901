// -*- C++ -*-
// Copyright (C) 2005-2010 Red Hat Inc.
// Copyright (C) 2007 Bull S.A.S
//
// This file is part of systemtap, and is free software.  You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.


#ifndef PARSE_H
#define PARSE_H

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

struct stapfile;

struct source_loc
{
  stapfile* file;
  unsigned line;
  unsigned column;
};

std::ostream& operator << (std::ostream& o, const source_loc& loc);

enum parse_context
  {
    con_unknown, con_probe, con_global, con_function, con_embedded
  };


enum token_type
  {
    tok_junk, tok_identifier, tok_operator, tok_string, tok_number,
    tok_embedded, tok_keyword
  };


struct token
{
  source_loc location;
  token_type type;
  std::string content;
  std::string msg; // for tok_junk
  void make_junk (std::string msg);
  const token* chain; // macro invocation that produced this token
};


std::ostream& operator << (std::ostream& o, const token& t);


typedef enum { ctx_library, ctx_local } macro_ctx;

/* structs from session.h: */
struct systemtap_session;
struct macrodecl {
  const token* tok; // NB: macrodecl owns its token
  std::string name;
  std::vector<std::string> formal_args;
  std::vector<const token*> body;
  macro_ctx context;

  // Used for identifying subclasses that represent e.g. parameter bindings.
  virtual bool is_closure() { return false; }

  macrodecl () : tok(0), context(ctx_local) { }
  virtual ~macrodecl ();
};


stapfile* parse (systemtap_session& s, std::istream& i, bool privileged, bool errs_as_warnings);
stapfile* parse (systemtap_session& s, const std::string& n, bool privileged, bool errs_as_warnings);

stapfile* parse_library_macros (systemtap_session& s, const std::string& n, bool errs_as_warnings);

#endif // PARSE_H

/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
