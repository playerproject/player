/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h> // for PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <locale.h>

#include "player.h"
#include "replace.h"
#include "configfile.h"
#include "deviceregistry.h"

extern int global_playerport;

#define COLOR_DATABASE "/usr/X11R6/lib/X11/rgb.txt"


///////////////////////////////////////////////////////////////////////////
// Useful macros for dumping parser errors
#define TOKEN_ERR(z, l) \
  PLAYER_ERROR2("%s:%d : " z, this->filename, l)
#define PARSE_ERR(z, l) \
  PLAYER_ERROR2("%s:%d : " z, this->filename, l)

// the isblank() macro is not standard - it's a GNU extension
// and it doesn't work for me, so here's an implementation - rtv
#ifndef isblank
  #define isblank(a) (a == ' ' || a == '\t')
#endif


///////////////////////////////////////////////////////////////////////////
// Default constructor
ConfigFile::ConfigFile() 
{
  this->filename = NULL;

  this->token_size = 0;
  this->token_count = 0;
  this->tokens = NULL;

  this->macro_count = 0;
  this->macro_size = 0;
  this->macros = NULL;

  this->entity_count = 0;
  this->entity_size = 0;
  this->entities = NULL;

  this->property_count = 0;
  this->property_size = 0;
  this->properties = NULL;

  // Set defaults units
  this->unit_length = 1.0;
  this->unit_angle = M_PI / 180;

  if(!setlocale(LC_ALL,"POSIX"))
    fputs("Warning: failed to setlocale(); config file may not be parse correctly\n", stderr);
}


///////////////////////////////////////////////////////////////////////////
// Destructor
ConfigFile::~ConfigFile()
{
  ClearProperties();
  ClearMacros();
  ClearEntities();
  ClearTokens();

  if (this->filename)
    free(this->filename);
}


///////////////////////////////////////////////////////////////////////////
// Load world from file
bool ConfigFile::Load(const char *filename)
{
  // Shouldnt call load more than once,
  // so this should be null.
  assert(this->filename == NULL);
  this->filename = strdup(filename);

  // Open the file
  FILE *file = fopen(this->filename, "r");
  if (!file)
  {
    PLAYER_ERROR2("unable to open world file %s : %s",
               this->filename, strerror(errno));
    //fclose(file);
    return false;
  }

  ClearTokens();
  
  // Read tokens from the file
  if (!LoadTokens(file, 0))
  {
    //DumpTokens();
    fclose(file);
    return false;
  }

  // Parse the tokens to identify entities
  if (!ParseTokens())
  {
    //DumpTokens();
    fclose(file);
    return false;
  }

  // Dump contents and exit if this file is meant for debugging only.
  if (ReadInt(0, "test", 0) != 0)
  {
    PLAYER_ERROR("this is a test file; quitting");
    DumpTokens();
    DumpMacros();
    DumpEntities();
    DumpProperties();
    fclose(file);
    return false;
  }
  
  // Work out what the length units are
  const char *unit = ReadString(0, "unit_length", "m");
  if (strcmp(unit, "m") == 0)
    this->unit_length = 1.0;
  else if (strcmp(unit, "cm") == 0)
    this->unit_length = 0.01;
  else if (strcmp(unit, "mm") == 0)
    this->unit_length = 0.001;

  // Work out what the angle units are
  unit = ReadString(0, "unit_angle", "degrees");
  if (strcmp(unit, "degrees") == 0)
    this->unit_angle = M_PI / 180;
  else if (strcmp(unit, "radians") == 0)
    this->unit_angle = 1;
  
  //DumpTokens();
  fclose(file);  
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Save world to file
bool ConfigFile::Save(const char *filename)
{
  // Debugging
  //DumpProperties();
  
  // If no filename is supplied, use default
  if (!filename)
    filename = this->filename;

  // Open file
  FILE *file = fopen(filename, "w+");
  if (!file)
  {
    PLAYER_ERROR2("unable to open world file %s : %s",
               filename, strerror(errno));
    return false;
  }

  // Write the current set of tokens to the file
  if (!SaveTokens(file))
  {
    fclose(file);
    return false;
  }

  fclose(file);
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Check for unused properties and print warnings
bool ConfigFile::WarnUnused()
{
  bool unused = false;
  for (int i = 0; i < this->property_count; i++)
  {
    CProperty *property = this->properties + i;
    if (!property->used)
    {
      unused = true;
      PLAYER_WARN3("configfile %s:%d : property [%s] is defined but not used",
                  this->filename, property->line, property->name);
    }
  }
  return unused;
}


///////////////////////////////////////////////////////////////////////////
// Load tokens from a file.
bool ConfigFile::LoadTokens(FILE *file, int include)
{
  int ch;
  int line;
  char token[256];
  
  line = 1;

  while (true)
  {
    ch = fgetc(file);
    if (ch == EOF)
      break;
    
    if ((char) ch == '#')
    {
      ungetc(ch, file);
      if (!LoadTokenComment(file, &line, include))
        return false;
    }
    else if (isalpha(ch))
    {
      ungetc(ch, file);
      if (!LoadTokenWord(file, &line, include))
        return false;
    }
    else if (strchr("+-.0123456789", ch))
    {
      ungetc(ch, file);
      if (!LoadTokenNum(file, &line, include))
        return false;
    }
    else if (isblank(ch))
    {
      ungetc(ch, file);
      if (!LoadTokenSpace(file, &line, include))
        return false;
    }
    else if (ch == '"')
    {
      ungetc(ch, file);
      if (!LoadTokenString(file, &line, include))
        return false;
    }
    else if (strchr("(", ch))
    {
      token[0] = ch;
      token[1] = 0;
      AddToken(TokenOpenEntity, token, include);
    }
    else if (strchr(")", ch))
    {
      token[0] = ch;
      token[1] = 0;
      AddToken(TokenCloseEntity, token, include);
    }
    else if (strchr("[", ch))
    {
      token[0] = ch;
      token[1] = 0;
      AddToken(TokenOpenTuple, token, include);
    }
    else if (strchr("]", ch))
    {
      token[0] = ch;
      token[1] = 0;
      AddToken(TokenCloseTuple, token, include);
    }
    else if (ch == '\n')
    {
      line++;
      AddToken(TokenEOL, "\n", include);
    }
    else
    {
      TOKEN_ERR("syntax error", line);
      return false;
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////
// Read in a comment token
bool ConfigFile::LoadTokenComment(FILE *file, int *line, int include)
{
  char token[1024];
  int len;
  int ch;

  len = 0;
  memset(token, 0, sizeof(token));
  
  while (true)
  {
    ch = fgetc(file);

    if (ch == EOF)
    {
      AddToken(TokenComment, token, include);
      return true;
    }
    else if (ch == '\n')
    {
      ungetc(ch, file);
      AddToken(TokenComment, token, include);
      return true;
    }
    else
      token[len++] = ch;
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Read in a word token
bool ConfigFile::LoadTokenWord(FILE *file, int *line, int include)
{
  char token[1024];
  int len;
  int ch;
  
  len = 0;
  memset(token, 0, sizeof(token));
  
  while (true)
  {
    ch = fgetc(file);

    if (ch == EOF)
    {
      AddToken(TokenWord, token, include);
      return true;
    }
    else if (isalpha(ch) || isdigit(ch) || strchr(".-_[]:", ch))
    {
      token[len++] = ch;
    }
    else
    {
      if (strcmp(token, "include") == 0)
      {
        ungetc(ch, file);
        AddToken(TokenWord, token, include);
        if (!LoadTokenInclude(file, line, include))
          return false;
      }
      else
      {
        ungetc(ch, file);
        AddToken(TokenWord, token, include);
      }
      return true;
    }
  }
  assert(false);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Load an include token; this will load the include file.
bool ConfigFile::LoadTokenInclude(FILE *file, int *line, int include)
{
  int ch;
  const char *filename;
  char *fullpath;
  
  ch = fgetc(file);

  if (ch == EOF)
  {
    TOKEN_ERR("incomplete include statement", *line);
    return false;
  }
  else if (!isblank(ch))
  {
    TOKEN_ERR("syntax error in include statement", *line);
    return false;
  }

  ungetc(ch, file);
  if (!LoadTokenSpace(file, line, include))
    return false;

  ch = fgetc(file);
  
  if (ch == EOF)
  {
    TOKEN_ERR("incomplete include statement", *line);
    return false;
  }
  else if (ch != '"')
  {
    TOKEN_ERR("syntax error in include statement", *line);
    return false;
  }

  ungetc(ch, file);
  if (!LoadTokenString(file, line, include))
    return false;

  // This is the basic filename
  filename = GetTokenValue(this->token_count - 1);

  // Now do some manipulation.  If its a relative path,
  // we append the path of the world file.
  if (filename[0] == '/' || filename[0] == '~')
  {
    fullpath = strdup(filename);
  }
  else if (this->filename[0] == '/' || this->filename[0] == '~')
  {
    // Note that dirname() modifies the contents, so
    // we need to make a copy of the filename.
    // There's no bounds-checking, but what the heck.
    char *tmp = strdup(this->filename);
    fullpath = (char*) malloc(PATH_MAX);
    memset(fullpath, 0, PATH_MAX);
    strcat( fullpath, dirname(tmp));
    strcat( fullpath, "/" ); 
    strcat( fullpath, filename );
    assert(strlen(fullpath) + 1 < PATH_MAX);
    free(tmp);
  }
  else
  {
    // Note that dirname() modifies the contents, so
    // we need to make a copy of the filename.
    // There's no bounds-checking, but what the heck.
    char *tmp = strdup(this->filename);
    fullpath = (char*) malloc(PATH_MAX);
    getcwd(fullpath, PATH_MAX);
    strcat( fullpath, "/" ); 
    strcat( fullpath, dirname(tmp));
    strcat( fullpath, "/" ); 
    strcat( fullpath, filename );
    assert(strlen(fullpath) + 1 < PATH_MAX);
    free(tmp);
  }

  // Open the include file
  FILE *infile = fopen(fullpath, "r");
  if (!infile)
  {
    PLAYER_ERROR2("unable to open include file %s : %s",
               fullpath, strerror(errno));
    free(fullpath);
    return false;
  }

  // Read tokens from the file
  if (!LoadTokens(infile, include + 1))
  {
    //DumpTokens();
    free(fullpath);
    return false;
  }

  free(fullpath);
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Read in a number token
bool ConfigFile::LoadTokenNum(FILE *file, int *line, int include)
{
  char token[1024];
  int len;
  int ch;
  
  len = 0;
  memset(token, 0, sizeof(token));
  
  while (true)
  {
    ch = fgetc(file);

    if (ch == EOF)
    {
      AddToken(TokenNum, token, include);
      return true;
    }
    else if (strchr("+-.0123456789", ch))
    {
      token[len++] = ch;
    }
    else
    {
      AddToken(TokenNum, token, include);
      ungetc(ch, file);
      return true;
    }
  }
  assert(false);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Read in a string token
bool ConfigFile::LoadTokenString(FILE *file, int *line, int include)
{
  int ch;
  int len;
  char token[1024];
  
  len = 0;
  memset(token, 0, sizeof(token));

  ch = fgetc(file);
      
  while (true)
  {
    ch = fgetc(file);

    if (ch == EOF || ch == '\n')
    {
      TOKEN_ERR("unterminated string constant", *line);
      return false;
    }
    else if (ch == '"')
    {
      AddToken(TokenString, token, include);
      return true;
    }
    else
    {
      token[len++] = ch;
    }
  }
  assert(false);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Read in a whitespace token
bool ConfigFile::LoadTokenSpace(FILE *file, int *line, int include)
{
  int ch;
  int len;
  char token[1024];
  
  len = 0;
  memset(token, 0, sizeof(token));
  
  while (true)
  {
    ch = fgetc(file);

    if (ch == EOF)
    {
      AddToken(TokenSpace, token, include);
      return true;
    }
    else if (isblank(ch))
    {
      token[len++] = ch;
    }
    else
    {
      AddToken(TokenSpace, token, include);
      ungetc(ch, file);
      return true;
    }
  }
  assert(false);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Save tokens to a file.
bool ConfigFile::SaveTokens(FILE *file)
{
  int i;
  CToken *token;
  
  for (i = 0; i < this->token_count; i++)
  {
    token = this->tokens + i;

    if (token->include > 0)
      continue;
    if (token->type == TokenString)
      fprintf(file, "\"%s\"", token->value);  
    else
      fprintf(file, "%s", token->value);
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Clear the token list
void ConfigFile::ClearTokens()
{
  int i;
  CToken *token;

  for (i = 0; i < this->token_count; i++)
  {
    token = this->tokens + i;
    free(token->value);
  }
  free(this->tokens);
  this->tokens = 0;
  this->token_size = 0;
  this->token_count = 0;
}


///////////////////////////////////////////////////////////////////////////
// Add a token to the token list
bool ConfigFile::AddToken(int type, const char *value, int include)
{
  if (this->token_count >= this->token_size)
  {
    this->token_size += 1000;
    this->tokens = (CToken*) realloc(this->tokens, this->token_size * sizeof(this->tokens[0]));
  }

  this->tokens[this->token_count].include = include;  
  this->tokens[this->token_count].type = type;
  this->tokens[this->token_count].value = strdup(value);
  this->token_count++;
  
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Set a token value in the token list
bool ConfigFile::SetTokenValue(int index, const char *value)
{
  assert(index >= 0 && index < this->token_count);

  free(this->tokens[index].value);
  this->tokens[index].value = strdup(value);
  
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Get the value of a token
const char *ConfigFile::GetTokenValue(int index)
{
  assert(index >= 0 && index < this->token_count);

  return this->tokens[index].value;
}


///////////////////////////////////////////////////////////////////////////
// Dump the token list (for debugging).
void ConfigFile::DumpTokens()
{
  int line;

  line = 1;
  printf("\n## begin tokens\n");
  printf("## %4d : ", line);
  for (int i = 0; i < this->token_count; i++)
  {
    if (this->tokens[i].value[0] == '\n')
      printf("[\\n]\n## %4d : %02d ", ++line, this->tokens[i].include);
    else
      printf("[%s] ", this->tokens[i].value);
  }
  printf("\n");
  printf("## end tokens\n");
}


///////////////////////////////////////////////////////////////////////////
// Parse tokens into entities and properties.
bool ConfigFile::ParseTokens()
{
  int i;
  int entity;
  int line;
  CToken *token;

  ClearEntities();
  ClearProperties();
  
  // Add in the "global" entity.
  entity = AddEntity(-1, "");
  line = 1;
  
  for (i = 0; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenWord:
        if (strcmp(token->value, "include") == 0)
        {
          if (!ParseTokenInclude(&i, &line))
            return false;
        }
        else if (strcmp(token->value, "define") == 0)
        {
          if (!ParseTokenDefine(&i, &line))
            return false;
        }
        else
        {
          if (!ParseTokenWord(entity, &i, &line))
            return false;
        }
        break;
      case TokenComment:
        break;
      case TokenSpace:
        break;
      case TokenEOL:
        line++;
        break;
      default:
        PARSE_ERR("syntax error 1", line);
        return false;
    }
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Parse an include statement
bool ConfigFile::ParseTokenInclude(int *index, int *line)
{
  int i;
  CToken *token;

  for (i = *index + 1; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenString:
        break;
      case TokenSpace:
        break;
      case TokenEOL:
        *index = i;
        (*line)++;
        return true;
      default:
        PARSE_ERR("syntax error in include statement", *line);
    }
  }
  PARSE_ERR("incomplete include statement", *line);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Parse a macro definition
bool ConfigFile::ParseTokenDefine(int *index, int *line)
{
  int i;
  int count;
  const char *macroname, *entityname;
  int starttoken;
  CToken *token;

  count = 0;
  macroname = NULL;
  entityname = NULL;
  starttoken = -1;

  for (i = *index + 1; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenWord:
        if (count == 0)
        {
          if (macroname == NULL)
            macroname = GetTokenValue(i);
          else if (entityname == NULL)
          {
            entityname = GetTokenValue(i);
            starttoken = i;
          }
          else
          {
            PARSE_ERR("extra tokens in macro definition", *line);
            return false;
          }
        }
        else
        {
          if (macroname == NULL)
          {
            PARSE_ERR("missing name in macro definition", *line);
            return false;
          }
          if (entityname == NULL)
          {
            PARSE_ERR("missing name in macro definition", *line);
            return false;
          }
        }
        break;
      case TokenOpenEntity:
        count++;
        break;
      case TokenCloseEntity:
        count--;
        if (count == 0)
        {
          AddMacro(macroname, entityname, *line, starttoken, i);
          *index = i;
          return true;
        }
        if (count < 0)
        {
          PARSE_ERR("misplaced ')'", *line);
          return false;
        }
        break;
      default:
        break;
    }
  }
  PARSE_ERR("missing ')'", *line);
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Parse something starting with a word; could be a entity or an property.
bool ConfigFile::ParseTokenWord(int entity, int *index, int *line)
{
  int i;
  CToken *token;

  for (i = *index + 1; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenComment:
        break;
      case TokenSpace:
        break;
      case TokenEOL:
        (*line)++;
        break;
      case TokenOpenEntity:
        return ParseTokenEntity(entity, index, line);
      case TokenNum:
      case TokenString:
      case TokenOpenTuple:
        return ParseTokenProperty(entity, index, line);
      default:
        PARSE_ERR("syntax error 2", *line);
        return false;
    }
  }
  
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Parse a entity from the token list.
bool ConfigFile::ParseTokenEntity(int entity, int *index, int *line)
{
  int i;
  int macro;
  int name;
  CToken *token;

  name = *index;
  macro = LookupMacro(GetTokenValue(name));
  
  // If the entity name is a macro...
  if (macro >= 0)
  {
    // This is a bit of a hack
    int nentity = this->entity_count;
    int mindex = this->macros[macro].starttoken;
    int mline = this->macros[macro].line;
    if (!ParseTokenEntity(entity, &mindex, &mline))
      return false;
    entity = nentity;

    for (i = *index + 1; i < this->token_count; i++)
    {
      token = this->tokens + i;

      switch (token->type)
      {
        case TokenOpenEntity:
          break;
        case TokenWord:
          if (!ParseTokenWord(entity, &i, line))
            return false;
          break;
        case TokenCloseEntity:
          *index = i;
          return true;
        case TokenComment:
          break;
        case TokenSpace:
          break;
        case TokenEOL:
          (*line)++;
          break;
        default:
          PARSE_ERR("syntax error 3", *line);
          return false;
      }
    }
    PARSE_ERR("missing ')'", *line);
  }
  // If the entity name is not a macro...
  else
  {
    for (i = *index + 1; i < this->token_count; i++)
    {
      token = this->tokens + i;

      switch (token->type)
      {
        case TokenOpenEntity:
          entity = AddEntity(entity, GetTokenValue(name));
          break;
        case TokenWord:
          if (!ParseTokenWord(entity, &i, line))
            return false;
          break;
        case TokenCloseEntity:
          *index = i;
          return true;
        case TokenComment:
          break;
        case TokenSpace:
          break;
        case TokenEOL:
          (*line)++;
          break;
        default:
          PARSE_ERR("syntax error 3", *line);
          return false;
      }
    }
    PARSE_ERR("missing ')'", *line);
  }
  return false;
}


///////////////////////////////////////////////////////////////////////////
// Parse an property from the token list.
bool ConfigFile::ParseTokenProperty(int entity, int *index, int *line)
{
  int i, property;
  int name, value, count;
  CToken *token;

  name = *index;
  value = -1;
  count = 0;
  
  for (i = *index + 1; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenNum:
        property = AddProperty(entity, GetTokenValue(name), *line);
        AddPropertyValue(property, 0, i);
        *index = i;
        return true;
      case TokenString:
        property = AddProperty(entity, GetTokenValue(name), *line);
        AddPropertyValue(property, 0, i);
        *index = i;
        return true;
      case TokenOpenTuple:
        property = AddProperty(entity, GetTokenValue(name), *line);
        if (!ParseTokenTuple(entity, property, &i, line))
          return false;
        *index = i;
        return true;
      case TokenSpace:
        break;
      default:
        PARSE_ERR("syntax error 4", *line);
        return false;
    }
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Parse a tuple.
bool ConfigFile::ParseTokenTuple(int entity, int property, int *index, int *line)
{
  int i, count;
  CToken *token;

  count = 0;
  
  for (i = *index + 1; i < this->token_count; i++)
  {
    token = this->tokens + i;

    switch (token->type)
    {
      case TokenNum:
        AddPropertyValue(property, count++, i);
        *index = i;
        break;
      case TokenString:
        AddPropertyValue(property, count++, i);
        *index = i;
        break;
      case TokenCloseTuple:
        *index = i;
        return true;
      case TokenSpace:
      case TokenEOL:
        break;
      default:
        PARSE_ERR("syntax error 5", *line);
        return false;
    }
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
// Clear the macro list
void ConfigFile::ClearMacros()
{
  free(this->macros);
  this->macros = NULL;
  this->macro_size = 0;
  this->macro_count = 0;
}


///////////////////////////////////////////////////////////////////////////
// Add a macro
int ConfigFile::AddMacro(const char *macroname, const char *entityname,
                         int line, int starttoken, int endtoken)
{
  if (this->macro_count >= this->macro_size)
  {
    this->macro_size += 100;
    this->macros = (CMacro*)
      realloc(this->macros, this->macro_size * sizeof(this->macros[0]));
  }

  int macro = this->macro_count;
  this->macros[macro].macroname = macroname;
  this->macros[macro].entityname = entityname;
  this->macros[macro].line = line;
  this->macros[macro].starttoken = starttoken;
  this->macros[macro].endtoken = endtoken;
  this->macro_count++;
  
  return macro;
}


///////////////////////////////////////////////////////////////////////////
// Lookup a macro by name
// Returns -1 if there is no macro with this name.
int ConfigFile::LookupMacro(const char *macroname)
{
  int i;
  CMacro *macro;
  
  for (i = 0; i < this->macro_count; i++)
  {
    macro = this->macros + i;
    if (strcmp(macro->macroname, macroname) == 0)
      return i;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
// Dump the macro list for debugging
void ConfigFile::DumpMacros()
{
  printf("\n## begin macros\n");
  for (int i = 0; i < this->macro_count; i++)
  {
    CMacro *macro = this->macros + i;

    printf("## [%s][%s]", macro->macroname, macro->entityname);
    for (int j = macro->starttoken; j <= macro->endtoken; j++)
    {
      if (this->tokens[j].type == TokenEOL)
        printf("[\\n]");
      else
        printf("[%s]", GetTokenValue(j));
    }
    printf("\n");
  }
  printf("## end macros\n");
}


///////////////////////////////////////////////////////////////////////////
// Clear the entity list
void ConfigFile::ClearEntities()
{
  free(this->entities);
  this->entities = NULL;
  this->entity_size = 0;
  this->entity_count = 0;
}


///////////////////////////////////////////////////////////////////////////
// Add a entity
int ConfigFile::AddEntity(int parent, const char *type)
{
  if (this->entity_count >= this->entity_size)
  {
    this->entity_size += 100;
    this->entities = (CEntity*)
      realloc(this->entities, this->entity_size * sizeof(this->entities[0]));
  }

  int entity = this->entity_count;
  this->entities[entity].parent = parent;
  this->entities[entity].type = type;
  this->entity_count++;
  
  return entity;
}


///////////////////////////////////////////////////////////////////////////
// Get the number of entities
int ConfigFile::GetEntityCount()
{
  return this->entity_count;
}


///////////////////////////////////////////////////////////////////////////
// Get a entity's parent entity
int ConfigFile::GetEntityParent(int entity)
{
  if (entity < 0 || entity >= this->entity_count)
    return -1;
  return this->entities[entity].parent;
}


///////////////////////////////////////////////////////////////////////////
// Get a entity (returns the entity type value)
const char *ConfigFile::GetEntityType(int entity)
{
  if (entity < 0 || entity >= this->entity_count)
    return NULL;
  return this->entities[entity].type;
}

///////////////////////////////////////////////////////////////////////////
// Lookup a entity number by type name
// Returns -1 if there is no entity with this type
int ConfigFile::LookupEntity(const char *type)
{
  for (int entity = 0; entity < GetEntityCount(); entity++)
  {
    if (strcmp(GetEntityType(entity), type) == 0)
      return entity;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
// Dump the entity list for debugging
void ConfigFile::DumpEntities()
{
  printf("\n## begin entities\n");
  for (int i = 0; i < this->entity_count; i++)
  {
    CEntity *entity = this->entities + i;

    printf("## [%d][%d]", i, entity->parent);
    printf("[%s]\n", entity->type);
  }
  printf("## end entities\n");
}


///////////////////////////////////////////////////////////////////////////
// Clear the property list
void ConfigFile::ClearProperties()
{
  int i;
  CProperty *property;

  for (i = 0; i < this->property_count; i++)
  {
    property = this->properties + i;
    free(property->values);
  }
  free(this->properties);
  this->properties = NULL;
  this->property_size = 0;
  this->property_count = 0;
}


///////////////////////////////////////////////////////////////////////////
// Add an property
int ConfigFile::AddProperty(int entity, const char *name, int line)
{
  int i;
  CProperty *property;
  
  // See if this property already exists; if it does, we dont need to
  // add it again.
  for (i = 0; i < this->property_count; i++)
  {
    property = this->properties + i;
    if (property->entity != entity)
      continue;
    if (strcmp(property->name, name) == 0)
      return i;
  }

  // Expand property array if necessary.
  if (i >= this->property_size)
  {
    this->property_size += 100;
    this->properties = (CProperty*)
      realloc(this->properties, this->property_size * sizeof(this->properties[0]));
  }

  property = this->properties + i;
  memset(property, 0, sizeof(CProperty));
  property->entity = entity;
  property->name = name;
  property->value_count = 0;
  property->values = NULL;
  property->line = line;
  property->used = false;

  this->property_count++;

  return i;
}


///////////////////////////////////////////////////////////////////////////
// Add a property value
void ConfigFile::AddPropertyValue(int property, int index, int value_token)
{
  assert(property >= 0);
  CProperty *pproperty = this->properties + property;

  // Expand the array if it's too small
  if (index >= pproperty->value_count)
  {
    pproperty->value_count = index + 1;
    pproperty->values = (int*) realloc(pproperty->values, pproperty->value_count * sizeof(int));
  }

  // Set the relevant value
  pproperty->values[index] = value_token;
}


///////////////////////////////////////////////////////////////////////////
// Get a property 
int ConfigFile::GetProperty(int entity, const char *name)
{
  // Find first instance of property
  for (int i = 0; i < this->property_count; i++)
  {
    CProperty *property = this->properties + i;
    if (property->entity != entity)
      continue;
    if (strcmp(property->name, name) == 0)
      return i;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
// Set the value of an property
void ConfigFile::SetPropertyValue(int property, int index, const char *value)
{
  //assert(property >= 0 && property < this->property_count);
  CProperty *pproperty = this->properties + property;
  assert(index >= 0 && index < pproperty->value_count);

  // Set the relevant value
  SetTokenValue(pproperty->values[index], value);
}


///////////////////////////////////////////////////////////////////////////
// Get the value of an property 
const char *ConfigFile::GetPropertyValue(int property, int index)
{
  assert(property >= 0);
  CProperty *pproperty = this->properties + property;
  
  if(index >= pproperty->value_count)
    return NULL;

  pproperty->used = true;
  return GetTokenValue(pproperty->values[index]);
}


///////////////////////////////////////////////////////////////////////////
// Dump the property list for debugging
void ConfigFile::DumpProperties()
{
  printf("\n## begin properties\n");
  for (int i = 0; i < this->property_count; i++)
  {
    CProperty *property = this->properties + i;
    CEntity *entity = this->entities + property->entity;
    
    printf("## [%d]", property->entity);
    printf("[%s]", entity->type);
    printf("[%s]", property->name);
    for (int j = 0; j < property->value_count; j++)
      printf("[%s]", GetTokenValue(property->values[j]));
    printf("\n");
  }
  printf("## end properties\n");
}


///////////////////////////////////////////////////////////////////////////
// Read a string
const char *ConfigFile::ReadString(int entity, const char *name, const char *value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return GetPropertyValue(property, 0);
}


///////////////////////////////////////////////////////////////////////////
// Write a string
void ConfigFile::WriteString(int entity, const char *name, const char *value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return;
  SetPropertyValue(property, 0, value);  
}


///////////////////////////////////////////////////////////////////////////
// Read an int
int ConfigFile::ReadInt(int entity, const char *name, int value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atoi(GetPropertyValue(property, 0));
}


///////////////////////////////////////////////////////////////////////////
// Write an int
void ConfigFile::WriteInt(int entity, const char *name, int value)
{
  char default_str[64];
  snprintf(default_str, sizeof(default_str), "%d", value);
  WriteString(entity, name, default_str);
}


///////////////////////////////////////////////////////////////////////////
// Read a float
double ConfigFile::ReadFloat(int entity, const char *name, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, 0));
}


///////////////////////////////////////////////////////////////////////////
// Read a length (includes unit conversion)
double ConfigFile::ReadLength(int entity, const char *name, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, 0)) * this->unit_length;
}


///////////////////////////////////////////////////////////////////////////
// Write a length (includes units conversion)
void ConfigFile::WriteLength(int entity, const char *name, double value)
{
  char default_str[64];
  snprintf(default_str, sizeof(default_str), "%.3f", value / this->unit_length);
  WriteString(entity, name, default_str);
}


///////////////////////////////////////////////////////////////////////////
// Read an angle (includes unit conversion)
double ConfigFile::ReadAngle(int entity, const char *name, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, 0)) * this->unit_angle;
}


///////////////////////////////////////////////////////////////////////////
// Read a color (included text -> RGB conversion).
// We look up the color in one of the common color databases.
uint32_t ConfigFile::ReadColor(int entity, const char *name, uint32_t value)
{
  int property;
  const char *color;
  
  property = GetProperty(entity, name);
  if (property < 0)
    return value;
  color = GetPropertyValue(property, 0);

  return LookupColor(color);
}


///////////////////////////////////////////////////////////////////////////
// Read a file name.
// Always returns an absolute path.
// If the filename is entered as a relative path, we prepend
// the world files path to it.
const char *ConfigFile::ReadFilename(int entity, const char *name, const char *value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;

  const char *filename = GetPropertyValue(property, 0);
  
  if( filename[0] == '/' || filename[0] == '~' )
    return filename;

  else if (this->filename[0] == '/' || this->filename[0] == '~')
  {
    // Note that dirname() modifies the contents, so
    // we need to make a copy of the filename.
    // There's no bounds-checking, but what the heck.
    char *tmp = strdup(this->filename);
    char *fullpath = (char*) malloc(PATH_MAX);
    memset(fullpath, 0, PATH_MAX);
    strcat( fullpath, dirname(tmp));
    strcat( fullpath, "/" ); 
    strcat( fullpath, filename );
    assert(strlen(fullpath) + 1 < PATH_MAX);

    SetPropertyValue(property, 0, fullpath);
    
    free(fullpath);
    free(tmp);
  }
  else
  {
    // Prepend the path
    // Note that dirname() modifies the contents, so
    // we need to make a copy of the filename.
    // There's no bounds-checking, but what the heck.
    char *tmp = strdup(this->filename);
    char *fullpath = (char*) malloc(PATH_MAX);
    getcwd(fullpath, PATH_MAX);
    strcat( fullpath, "/" ); 
    strcat( fullpath, dirname(tmp));
    strcat( fullpath, "/" ); 
    strcat( fullpath, filename );
    assert(strlen(fullpath) + 1 < PATH_MAX);

    SetPropertyValue(property, 0, fullpath);
    
    free(fullpath);
    free(tmp);
  }

  filename = GetPropertyValue(property, 0);
  assert(filename[0] == '/' || filename[0] == '~');

  return filename;
}


///////////////////////////////////////////////////////////////////////////
// Read a string from a tuple
const char *ConfigFile::ReadTupleString(int entity, const char *name,
                                        int index, const char *value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return GetPropertyValue(property, index);
}


///////////////////////////////////////////////////////////////////////////
// Write a string to a tuple
void ConfigFile::WriteTupleString(int entity, const char *name,
                                  int index, const char *value)
{
  int property = GetProperty(entity, name);
  /* TODO
  if (property < 0)
    property = InsertProperty(entity, name);
  */
  SetPropertyValue(property, index, value);  
}


///////////////////////////////////////////////////////////////////////////
// Read a float from a tuple
double ConfigFile::ReadTupleFloat(int entity, const char *name,
                                  int index, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, index));
  
}


///////////////////////////////////////////////////////////////////////////
// Write a float to a tuple
void ConfigFile::WriteTupleFloat(int entity, const char *name,
                                 int index, double value)
{
  char default_str[64];
  snprintf(default_str, sizeof(default_str), "%.3f", value);
  WriteTupleString(entity, name, index, default_str);
}


///////////////////////////////////////////////////////////////////////////
// Read a length from a tuple (includes unit conversion)
double ConfigFile::ReadTupleLength(int entity, const char *name,
                                   int index, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, index)) * this->unit_length;
}


///////////////////////////////////////////////////////////////////////////
// Write a length to a tuple (includes unit conversion)
void ConfigFile::WriteTupleLength(int entity, const char *name,
                                 int index, double value)
{
  char default_str[64];
  snprintf(default_str, sizeof(default_str), "%.3f", value / this->unit_length);
  WriteTupleString(entity, name, index, default_str);
}


///////////////////////////////////////////////////////////////////////////
// Read an angle from a tuple (includes unit conversion)
double ConfigFile::ReadTupleAngle(int entity, const char *name,
                                  int index, double value)
{
  int property = GetProperty(entity, name);
  if (property < 0)
    return value;
  return atof(GetPropertyValue(property, index)) * this->unit_angle;
}


///////////////////////////////////////////////////////////////////////////
// Write an angle to a tuple (includes unit conversion)
void ConfigFile::WriteTupleAngle(int entity, const char *name,
                                 int index, double value)
{
  char default_str[64];
  snprintf(default_str, sizeof(default_str), "%.3f", value / this->unit_angle);
  WriteTupleString(entity, name, index, default_str);
}



///////////////////////////////////////////////////////////////////////////
// Read a color (included text -> RGB conversion).
// We look up the color in one of the common color databases.
uint32_t ConfigFile::ReadTupleColor(int entity, const char *name, int index, uint32_t value)
{
  int property;
  const char *color;

  property = GetProperty(entity, name);
  if (property < 0)
    return value;
  color = GetPropertyValue(property, index);
  if (!color)
    return value;

  return LookupColor(color);
}


///////////////////////////////////////////////////////////////////////////
// Look up the color in a data based (transform color name -> color value).
uint32_t ConfigFile::LookupColor(const char *name)
{
  FILE *file;
  const char *filename;

  filename = COLOR_DATABASE;
  file = fopen(filename, "r");
  if (!file)
  {
    PLAYER_ERROR2("unable to open color database %s : %s",
                  filename, strerror(errno));
    fclose(file);
    return 0xFFFFFF;
  }
  
  while (true)
  {
    char line[1024];
    if (!fgets(line, sizeof(line), file))
      break;

    // it's a macro or comment line - ignore the line
    if (line[0] == '!' || line[0] == '#' || line[0] == '%') 
      continue;

    // Trim the trailing space
    while (strchr(" \t\n", line[strlen(line)-1]))
      line[strlen(line)-1] = 0;

    // Read the color
    int r, g, b;
    int chars_matched = 0;
    sscanf( line, "%d %d %d %n", &r, &g, &b, &chars_matched );
      
    // Read the name
    char* nname = line + chars_matched;

    // If the name matches
    if (strcmp(nname, name) == 0)
    {
      fclose(file);
      return ((r << 16) | (g << 8) | b);
    }
  }
  PLAYER_WARN1("unable to find color [%s]; using default (red)", name);
  fclose(file);
  return 0xFF0000;
}

// Parse the "devices" option in the given section.  Returns the number
// of device ids if a valid "devices" section is found, and -1 otherwise.
// 0 is returned, then ids will point to a malloc()ed list of the parsed
// ids (which the caller should free()), in the order they were given.
int
ConfigFile::ParseDeviceIds(int section, player_device_id_t** ids)
{
  int property_idx;
  CProperty* property;
  const char *str;
  int port, ind;
  char s[128];
  player_interface_t interface;

  if((property_idx = GetProperty(section,"devices")) < 0)
  {
    PLAYER_ERROR1("section [%d]: missing devices entry", section);
    return -1;
  }

  property = this->properties + property_idx;

  assert(*ids = (player_device_id_t*)malloc(property->value_count * 
                                            sizeof(player_device_id_t)));

  for(int i=0; i<property->value_count; i++)
  {
    assert(str = GetPropertyValue(property_idx,i));
    // Look for port:interface:index
    if(sscanf(str, "%d:%127[^:]:%d", &port, s, &ind) < 3)
    {
      port = global_playerport;

      // Look for interface:index
      if(sscanf(str, "%127[^:]:%d", s, &ind) < 2)
      {
        PLAYER_ERROR1("section [%d]: syntax error in interface field", section);
        free(*ids);
        return -1;
      }
    }

    // Find the interface
    if(::lookup_interface(s, &interface) != 0)
    {
      PLAYER_ERROR2("section [%d]: unknown interface: [%s]", section, s);
      free(*ids);
      return -1;
    }

    (*ids)[i].code = interface.code;
    (*ids)[i].index = ind;
    (*ids)[i].port = port;
  }

  return(property->value_count);
}

// Given a list of ids (e.g., one returned by ParseDeviceIds()) of length
// num_ids, if there exists an i'th id with the given code, fills in id
// appropriately, and returns 0.
//
// "Consumes" the selected id in the list, by setting the port to 0.  Thus,
// after calling ReadDeviceId() for each supported interface, you can call
// UnusedIds() to determine whether the user gave any extra interfaces.
//
// Returns -1 if no such id can be found.
int
ConfigFile::ReadDeviceId(player_device_id_t* id, player_device_id_t* ids,
                         int num_ids, int code, int i)
{
  int j,k;

  for(k=0,j=0;j<num_ids;j++)
  {
    if(ids[j].code == code || code == -1)
    {
      if(k==i)
      {
        *id = ids[j];
        // consume this id
        ids[j].port = 0;
        return(0);
      }
      else
        k++;
    }
  }
  return(-1);
}

// Given a list of ids (e.g., one returned by ParseDeviceIds()) of length
// num_ids, tells whether there remain any "unused" ids.  An id is unused
// if its port is nonzero.
int
ConfigFile::UnusedIds(int section, player_device_id_t* ids, int num_ids)
{
  int unused = 0;

  for(int i=0;i<num_ids;i++)
  {
    if(ids[i].port)
    {
      PLAYER_ERROR3("section [%d]: unused device ID %s:%d",
                    section,::lookup_interface_name(0,ids[i].code),ids[i].index);
      unused = 1;
    }
  }
  return(unused);
}
