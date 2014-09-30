%option yylineno
%option noyywrap
%option warn
%option debug
%option pointer
%top{
/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <string>
#include <vector>

// The following includes are used in flex generated source. Because we use
// namespaces, include them first to avoid namespace errors when these files
// are included from flex generated source.
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <ggadget/logger.h>

// These macros make the '{' and '}' paired in flex source code.
#define BEGIN_NS(ns) namespace ns {
#define END_NS }

BEGIN_NS(ggadget)
BEGIN_NS(js)

}

%{

#undef YY_INPUT
#define YY_INPUT(a,b,c) (b = ScriptInput(a,c))

static const char *input_script_pos = NULL;
static bool new_line_appended = false;

static int ScriptInput(char *buf, int max_size) {
  int result = 0;
  while (*input_script_pos && max_size > 0) {
    if (*input_script_pos != '\r') {
      max_size--;
      result++;
      *buf++ = *input_script_pos;
    }
    input_script_pos++;
  }
  // Append a '\n' if the file is not ended with a '\n'.
  if (!new_line_appended && max_size > 0 &&
      !*input_script_pos && input_script_pos[-1] != '\n') {
    result++;
    *buf++ = '\n';
    new_line_appended = true;
  }
  return result;
}

#undef ECHO
#define ECHO ScriptOutput(yytext)

enum State {
  NORMAL,
  OPTIONS_ONLY, // After options( or detailsViewData(
  OPTIONS_ITEM, // After options.item( or detailsViewData.item(
  OPTIONS_DEFAULT_VALUE, // After options.defaultValue( or
                         // detailsViewData.defaultValue(
  FUNCTION_DECL, // After function name(...) {
  FUNCTION_EXPR, // After function {name}(...) { in an expression context
};

struct StackEntry {
  StackEntry(State a_state)
      : state(a_state), assigned(false), paren_count(0) {
  }
  std::string buffer;
  // Buffers the functions that was declared in inner blocks.
  std::string function_decls;
  State state;
  bool assigned;
  int paren_count;
};
static std::vector<StackEntry> stack;
static const char *input_filename = NULL;
static std::string output;
static bool give_up = false;

static StackEntry &GetStackTop() {
  ASSERT(!stack.empty());
  return *(stack.end() - 1);
}

static State GetState() {
  return stack.empty() ? NORMAL : GetStackTop().state;  
}

static void ScriptOutput(const char *str) {
  if (GetState() != NORMAL)
    GetStackTop().buffer += str;
  else
    output += str;
}

static void PushState(State state) {
  stack.push_back(StackEntry(state));
  if (yy_flex_debug)
    DLOG("%d:PUSHSTATE(%zd): state=%d", yylineno, stack.size(), state);
}

static int GetParenCount() {
  return GetStackTop().paren_count;
}

static void IncreaseParenCount() {
  if (!stack.empty()) {
    GetStackTop().paren_count++;
    if (yy_flex_debug)
      DLOG("%d:INCPAREN(%zd): %d", yylineno, stack.size(),
           GetStackTop().paren_count);
  }
}

static void DecreaseParenCount() {
  if (!stack.empty()) {
    GetStackTop().paren_count--;
    if (yy_flex_debug)
      DLOG("%d:DECPAREN(%zd): %d", yylineno, stack.size(),
           GetStackTop().paren_count);
  }
}

static bool IsAssigned() {
  return !stack.empty() && GetStackTop().assigned;
}

static void SetAssigned() {
  GetStackTop().assigned = true;
}

// NOTE: This function compares strings case-Sensitively.
static void ReplaceLast(const char *src, const char *dest) {
  std::string &buffer = GetStackTop().buffer;
  size_t pos = buffer.find_last_of(src);
  if (pos != buffer.npos)
    buffer.replace(pos, strlen(src), dest);
}

// NOTE: This function compares strings case-INSensitively.
static void ReplaceFirst(const char *src, const char *dest) {
  std::string &buffer = GetStackTop().buffer;
  size_t srclen = strlen(src);
  if (buffer.size() > srclen) {
    size_t upper_bound = buffer.size() - srclen;
    for (size_t i = 0; i < upper_bound; i++) {
      if (strncasecmp(buffer.c_str() + i, src, srclen) == 0) {
        buffer.replace(i, srclen, dest);
        break;
      }
    }
  }
}

static void PopState() {
  State state = GetState();
  if (IsAssigned()) {
    ScriptOutput(")");
    switch (state) {
      case OPTIONS_ONLY:
        ReplaceFirst("(", ".putValue(");
        break;
      case OPTIONS_ITEM:
        ReplaceFirst("item", "putValue");
        break;
      case OPTIONS_DEFAULT_VALUE:
        ReplaceFirst("defaultValue", "putDefaultValue");
        break;
      default:
        ASSERT(false);
        break;
    }
  }
  std::string buffer = GetStackTop().buffer;
  std::string left_functions = GetStackTop().function_decls;
  if (!left_functions.empty()) {
    LOG("%s: File contains unpaired '(', '[' or '{'s.", input_filename);
    buffer += left_functions;
  }

  stack.pop_back();
  if (yy_flex_debug) {
    DLOG("%d:POPSTATE(%zd): state=%d buf=%s", yylineno, stack.size(),
         state, buffer.c_str());
  }
  if (state == FUNCTION_DECL && !stack.empty() && GetParenCount() > 1) {
    // This function is declared in inner block of another function.
    GetStackTop().function_decls += buffer;
    LOG("%s:%d: Non-standard JScript grammar (function declared in"
        " inner blocks): %s", input_filename, yylineno, buffer.c_str());
  } else {
    ScriptOutput(buffer.c_str());
  }
}

static void PopAllPairedStates() {
  while (!stack.empty() && GetParenCount() == 0)
    PopState();
}

static void PopAllStates() {
  while (!stack.empty())
    PopState();
}

static void CloseParen();

%}

COMMENT \/((\*[^*]*\*+([^*/][^*]*\*+)*\/)|(\/.*\n))
WS ([ \t\r\n]|{COMMENT})*
BINARY_OP [\.<>=\*-+\^&\|%\?:]
STRING \"([^\n\\\"]|\\(.|\n))*[\"\n]
SSTRING \'([^\n\\\']|\\(.|\n))*[\'\n]
REGEXP \/([^\n\\\/\*]|\\(.|\n))([^\n\\\/]|\\(.|\n))*[\/\n]
FUNC_PARAM {WS}\(([a-zA-Z0-9_]+|{WS}|,)*\){WS}

%x OPTIONS
%x AFTER_OPTIONS_PARAM
%x ASSIGN_RIGHT

%%

\xEF\xBB\xBF ECHO;
\xE3\x80\x80 ScriptOutput("  ");

{STRING} ECHO;
{SSTRING} ECHO;
{COMMENT} ECHO;

[\(\[\{,=]{WS}{REGEXP} {
  ECHO;
  if (strchr("([{", yytext[0]))
    IncreaseParenCount();
}

\/>|<\/ {
  LOG("%s:%d: Can't massage JS containing E4X grammar",
      input_filename, yylineno);
  give_up = true;
  PopAllStates();
  YY_FLUSH_BUFFER;
  yyterminate();
}

[a-zA-Z0-9_][OoDd] ECHO;

[Oo][Pp][Tt][Ii][Oo][Nn][Ss] |
[Dd][Ee][Tt][Aa][Ii][Ll][Ss][Vv][Ii][Ee][Ww][Dd][Aa][Tt][Aa] {
  // options or detailsViewData
  BEGIN(OPTIONS);
  yymore();
}

<OPTIONS>{WS}\( {
  BEGIN(INITIAL);
  PushState(OPTIONS_ONLY);
  ECHO;
  IncreaseParenCount();
}

<OPTIONS>{WS}\.{WS}[Ii][Tt][Ee][Mm]{WS}\( {
  // options.item or detailsViewData.item
  BEGIN(INITIAL);
  PushState(OPTIONS_ITEM);
  ECHO;
  IncreaseParenCount();
}

<OPTIONS>{WS}\.{WS}[Dd][Ee][Ff][Aa][Uu][Ll][Tt][Vv][Aa][Ll][Uu][Ee]{WS}\( {
  // options.defaultValue or detailsViewData.defaultValue
  BEGIN(INITIAL);
  PushState(OPTIONS_DEFAULT_VALUE);
  ECHO;
  IncreaseParenCount();
}

<OPTIONS>(.|\n) {
  BEGIN(INITIAL);
  // In <OPTIONS> state, yytext contains the last matched string because
  // yymore() was called.
  unput(yytext[yyleng - 1]);
  yytext[yyleng - 1] = '\0';
  ECHO;
}

\(|\{|\[ {
  ECHO;
  IncreaseParenCount();
}

\)|\}|\] {
  PopAllPairedStates();
  ECHO;
  CloseParen();
}

<AFTER_OPTIONS_PARAM>{WS}=/[^=] {
  // Mathes ...= but not ...==.
  ECHO;
  LOG("%s:%d: Old JScript grammar: %s... Converted to %s",
      input_filename, yylineno, GetStackTop().buffer.c_str(),
      GetState() == OPTIONS_DEFAULT_VALUE ? "putDefaultValue()" : "putValue()");
  ReplaceLast(")", ",");
  ReplaceLast("=", "");
  SetAssigned();
  BEGIN(ASSIGN_RIGHT);
}

<AFTER_OPTIONS_PARAM>.|\n {
  BEGIN(INITIAL);
  PopState();
  unput(yytext[0]);
}

<ASSIGN_RIGHT>{WS}\n {
  // Avoid [,\n] rule in this case.
  ECHO;
  BEGIN(INITIAL);
}

<ASSIGN_RIGHT>. {
  BEGIN(INITIAL);
  unput(yytext[0]);
}

({BINARY_OP}|[\/,]){WS}function{WS}[a-zA-Z0-9_]*{FUNC_PARAM}\{ |
{WS}function{WS}{FUNC_PARAM}\{ {
  PushState(FUNCTION_EXPR);
  ECHO;
  IncreaseParenCount();
}

{WS}function{WS}[a-zA-Z0-9_]+{FUNC_PARAM}\{ {
  PopAllPairedStates();
  PushState(FUNCTION_DECL);
  ECHO;
  IncreaseParenCount();
}

{BINARY_OP}{WS}\n ECHO; // Statement should continue.
\/\n ECHO; // BINARY_OP doesn't include '/' which is specially treated.
\/[ \t]{WS}\n ECHO;  // Distinguish comments from the '/' operator.
\n{WS}{BINARY_OP} ECHO; // Statement should continue.
\n{WS}\//[^*/] ECHO; // Distinguish comments from the '/' operator.

[,;\n] {
  // JavaScript allow lines not ended with ';'.
  PopAllPairedStates();
  ECHO;
}

\}{WS};{WS}else({WS}|\{) {
  PopAllPairedStates();
  CloseParen();
  // JScript allows ';' between '}' and 'else', which is invalid in standard JS.
  LOG("%s:%d: Old JScript grammar: %s...", input_filename, yylineno, yytext);
  std::string text(yytext);
  text[text.find(';')] = ' ';
  ScriptOutput(text.c_str());
  if (text[text.size() - 1] == '{')
    IncreaseParenCount();
}

<<EOF>> {
  PopAllPairedStates();
  if (stack.size() != 1) {
    LOG("%s: File contains unpaired '(', '[' or '{'s.", input_filename);
  }
  PopAllStates();
  YY_FLUSH_BUFFER;
  yyterminate();
}

<*>.|\n ECHO;

%%

static void CloseParen() {
  State state = GetState();
  if ((state == FUNCTION_DECL || state == FUNCTION_EXPR) &&
      GetParenCount() == 2) {
    // Move all functions declared in inner blocks to here. 
    ScriptOutput(GetStackTop().function_decls.c_str());
    GetStackTop().function_decls.clear();
    DecreaseParenCount();
  } else {
    if (state != NORMAL) {
      DecreaseParenCount();
      if (yytext[0] == ')' && GetParenCount() == 0 && !IsAssigned())
        BEGIN(AFTER_OPTIONS_PARAM);
    }
  }
}

std::string MassageJScript(const char *input, bool debug,
                           const char *filename, int lineno) {
  give_up = false;
  yy_flex_debug = debug;
#ifdef _DEBUG
  FILE *input_debug_file = fopen("massage_input.js", "w");
  if (input_debug_file) {
    fputs(input, input_debug_file);
    fclose(input_debug_file);
  }
#endif

  if (!input || !*input)
    return std::string();

  input_script_pos = input;
  input_filename = filename;
  new_line_appended = false;
  yyset_lineno(lineno);
  output.clear();
  stack.clear();

  BEGIN(INITIAL);
  // Create a fake global function context.
  PushState(FUNCTION_DECL);
  IncreaseParenCount();
  while (yylex());

#ifdef _DEBUG
  FILE *output_debug_file = fopen("massage_result.js", "w");
  if (output_debug_file) {
    fputs(output.c_str(), output_debug_file);
    fclose(output_debug_file);
  }
#endif

  return give_up ? std::string(input) : output;
}

END_NS // namespace js
END_NS // namespace ggadget
