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

#ifndef CONFFILE_H
#define CONFFILE_H

#include <stdio.h>
#include "playercommon.h"
#include "player.h"


/// @brief Class for loading configuration file information.
///
/// This class is used to load settings from a configuration text
/// file.  Ths file is dividing into sections, with section having a
/// set of key/value fields.
/// Example file format is as follows:
/// @verbatim
/// # This is a comment
/// section_name
/// (
///   key1  0             
///   key2 "foo"          
///   key3 ["foo" "bar"]  
/// )
/// @endverbatim

class ConfigFile
{
  /// @brief Standard constructor
  public: ConfigFile();

  /// @brief Standard destructor
  public: ~ConfigFile();

  /// @brief Load config from file
  /// @param filename Name of file; can be relative or fully qualified path.
  /// @returns Returns true on success.
  public: bool Load(const char *filename);

  // Save config back into file
  // Set filename to NULL to save back into the original file
  private: bool Save(const char *filename);

  /// @brief Check for unused fields and print warnings.
  /// @returns Returns true if there are unused fields.
  public: bool WarnUnused();

  /// @brief Read a string value
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the field value.
  public: const char *ReadString(int section, 
                                 const char *name, 
                                 const char *value);

  // Write a string
  private: void WriteString(int section, 
                            const char *name, 
                            const char *value);

  /// @brief Read an integer value
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the field value.
  public: int ReadInt(int section, 
                      const char *name, 
                      int value);

  // Write an integer
  private: void WriteInt(int section, 
                         const char *name, 
                         int value);

  /// @brief Read a floating point (double) value.
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the field value.
  public: double ReadFloat(int section, 
                           const char *name, 
                           double value);

  // Write a float
  private: void WriteFloat(int section, 
                           const char *name, 
                           double value);

  /// @brief Read a length (includes unit conversion, if any).
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the field value.
  public: double ReadLength(int section, 
                            const char *name, 
                            double value);

  // Write a length (includes units conversion)
  private: void WriteLength(int section, 
                            const char *name, 
                            double value);
  
  /// @brief Read an angle (includes unit conversion).
  ///
  /// In the configuration file, angles are specified in degrees; this
  /// method will convert them to radians.
  ///
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file (radians).
  /// @returns Returns the field value.
  public: double ReadAngle(int section, const char *name, double value);

  /// @brief Read a color (includes text to RGB conversion)
  ///
  /// In the configuration file colors may be specified with sybolic
  /// names; e.g., "blue" and "red".  This function will convert them
  /// to an RGB value using the X11 rgb.txt file.
  ///
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file (RGB).
  /// @returns Returns the field value.
  public: uint32_t ReadColor(int section, 
                             const char *name, 
                             uint32_t value);

  /// @brief Read a filename.
  ///
  /// Always returns an absolute path.  If the filename is entered as
  /// a relative path, we prepend the config file's path to it.
  ///
  /// @param section Section to read.
  /// @param name Field name
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the field value.
  public: const char *ReadFilename(int section, 
                                   const char *name, 
                                   const char *value);

  /// @brief Get the number of values in a tuple.
  /// @param section Section to read.
  /// @param name Field name.
  public: int GetTupleCount(int section, const char *name);

  /// @brief Read a string from a tuple field
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the tuple element value.
  public: const char *ReadTupleString(int section, 
                                      const char *name,
                                      int index, 
                                      const char *value);
  
  // Write a string to a tuple
  private: void WriteTupleString(int section, 
                                const char *name,
                                int index, 
                                const char *value);
  
  /// @brief Read an integer from a tuple field
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the tuple element value.
  public: int ReadTupleInt(int section, 
                           const char *name,
                           int index, 
                           int value);

  // Write a int to a tuple
  private: void WriteTupleInt(int section, 
                             const char *name,
                             int index, 
                             int value);
  

  /// @brief Read a float (double) from a tuple field
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the tuple element value.
  public: double ReadTupleFloat(int section, 
                                const char *name,
                                int index, 
                                double value);

  // Write a float to a tuple
  private: void WriteTupleFloat(int section, 
                               const char *name,
                               int index, 
                               double value);

  /// @brief Read a length from a tuple (includes units conversion)
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the tuple element value.
  public: double ReadTupleLength(int section, 
                                 const char *name,
                                 int index, 
                                 double value);

  // Write a to a tuple length (includes units conversion)
  private: void WriteTupleLength(int section, 
                                const char *name,
                                int index, 
                                double value);

  /// @brief Read an angle form a tuple (includes units conversion)
  ///
  /// In the configuration file, angles are specified in degrees; this
  /// method will convert them to radians.
  ///
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)
  /// @param value Default value if the field is not present in the file.
  /// @returns Returns the tuple element value.
  public: double ReadTupleAngle(int section, 
                                const char *name,
                                int index, 
                                double value);

  // Write an angle to a tuple (includes units conversion)
  private: void WriteTupleAngle(int section, 
                               const char *name,
                               int index, 
                               double value);

  /// @brief Read a color (includes text to RGB conversion)
  ///
  /// In the configuration file colors may be specified with sybolic
  /// names; e.g., "blue" and "red".  This function will convert them
  /// to an RGB value using the X11 rgb.txt file.
  ///
  /// @param section Section to read.
  /// @param name Field name
  /// @param index Tuple index (zero-based)  
  /// @param value Default value if the field is not present in the file (RGB).
  /// @returns Returns the field value.
  public: uint32_t ReadTupleColor(int section, 
                                  const char *name,
                                  int index, 
                                  uint32_t value); 

  /// @brief Read a device id.
  ///
  /// Reads a device id from the named field of the given section.
  /// The returned id will match the given code, index and key values.
  //
  /// @param id ID field to be filled in.
  /// @param section File section.
  /// @param name Field name.
  /// @param code Interface type code (use 0 to match all interface types).
  /// @param index Tuple index (use -1 to match all indices).
  /// @param key Device key value (use NULL to match all key vales).
  /// @return Non-zero on error.
  public: int ReadDeviceId(player_device_id_t *id, int section, const char *name,
                           int code, int index, const char *key);

  /// @brief Get the number of sections.
  public: int GetSectionCount();

  /// @brief Get a section type name.
  public: const char *GetSectionType(int section);

  /// @brief Lookup a section number by section type name.
  /// @return Returns -1 if there is no section with this type.
  public: int LookupSection(const char *type);
  
  /// @brief Get a section's parent section.
  /// @returns Returns -1 if there is no parent.
  public: int GetSectionParent(int section);


  ////////////////////////////////////////////////////////////////////////////
  // Private methods used to load stuff from the config file
  
  // Load tokens from a file.
  private: bool LoadTokens(FILE *file, int include);

  // Read in a comment token
  private: bool LoadTokenComment(FILE *file, int *line, int include);

  // Read in a word token
  private: bool LoadTokenWord(FILE *file, int *line, int include);

  // Load an include token; this will load the include file.
  private: bool LoadTokenInclude(FILE *file, int *line, int include);

  // Read in a number token
  private: bool LoadTokenNum(FILE *file, int *line, int include);

  // Read in a string token
  private: bool LoadTokenString(FILE *file, int *line, int include);

  // Read in a whitespace token
  private: bool LoadTokenSpace(FILE *file, int *line, int include);

  // Save tokens to a file.
  private: bool SaveTokens(FILE *file);

  // Clear the token list
  private: void ClearTokens();

  // Add a token to the token list
  public: bool AddToken(int type, const char *value, int include);

  // Set a token in the token list
  private: bool SetTokenValue(int index, const char *value);

  // Get the value of a token
  private: const char *GetTokenValue(int index);

  // Dump the token list (for debugging).
  public: void DumpTokens();

  // Parse a line
  private: bool ParseTokens();

  // Parse an include statement
  private: bool ParseTokenInclude(int *index, int *line);

  // Parse a macro definition
  private: bool ParseTokenDefine(int *index, int *line);

  // Parse an word (could be a section or an field) from the token list.
  private: bool ParseTokenWord(int section, int *index, int *line);

  // Parse a section from the token list.
  private: bool ParseTokenSection(int section, int *index, int *line);

  // Parse an field from the token list.
  private: bool ParseTokenField(int section, int *index, int *line);

  // Parse a tuple.
  private: bool ParseTokenTuple(int section, int field, 
                                int *index, int *line);

  // Clear the macro list
  private: void ClearMacros();

  // Add a macro
  private: int AddMacro(const char *macroname, const char *sectionname,
                        int line, int starttoken, int endtoken);

  // Lookup a macro by name
  // Returns -1 if there is no macro with this name.
  private: int LookupMacro(const char *macroname);

  // Dump the macro list for debugging
  private: void DumpMacros();

  // Clear the section list
  private: void ClearSections();

  // Add a section
  public: int AddSection(int parent, const char *type);

  // Dump the section list for debugging
  public: void DumpSections();

  // Clear the field list
  private: void ClearFields();

  // Add a field
  public: int AddField(int section, const char *name, int line);

  // Add a field value.
  public: void AddFieldValue(int field, int index, int value_token);
  
  // Get a field
  private: int GetField(int section, const char *name);

  // Get the number of elements for this field
  private: int GetFieldValueCount(int field);

  // Get the value of an field element
  // Set flag_used to true mark the field element as read.
  private: const char *GetFieldValue(int field, int index, bool flag_used = true);

  // Set the value of an field.
  private: void SetFieldValue(int field, int index, const char *value);

  // Dump the field list for debugging
  public: void DumpFields();

  // Look up the color in a data based (transform color name -> color value).
  private: uint32_t LookupColor(const char *name);

  /// Name of the file we loaded
  public: char *filename;

  // Token types.
  private: enum
    {
      TokenComment,
      TokenWord, TokenNum, TokenString,
      TokenOpenSection, TokenCloseSection,
      TokenOpenTuple, TokenCloseTuple,
      TokenSpace, TokenEOL
    };

  // Token structure.
  private: struct Token
  {
    // Non-zero if token is from an include file.
    int include;
    
    // Token type (enumerated value).
    int type;

    // Token value
    char *value;
  };

  // A list of tokens loaded from the file.
  // Modified values are written back into the token list.
  private: int token_size, token_count;
  private: Token *tokens;

  // Private macro class
  private: struct CMacro
  {
    // Name of macro
    const char *macroname;

    // Name of section
    const char *sectionname;

    // Line the macro definition starts on.
    int line;
    
    // Range of tokens in the body of the macro definition.
    int starttoken, endtoken;
  };

  // Macro list
  private: int macro_size;
  private: int macro_count;
  private: CMacro *macros;
  
  // Private section class
  private: struct Section
  {
    // Parent section
    int parent;

    // Type of section (i.e. position, laser, etc).
    const char *type;
  };

  // Section list
  private: int section_size;
  private: int section_count;
  private: Section *sections;

  // Private field class
  private: struct Field
  {
    // Index of section this field belongs to
    int section;

    // Name of field
    const char *name;
    
    // A list of token indexes
    int value_count;
    int *values;

    // Flag set if field value has been used
    bool *useds;

    // Line this field came from
    int line;
  };
  
  // Field list
  private: int field_size;
  private: int field_count;
  private: Field *fields;

  // Conversion units
  private: double unit_length;
  private: double unit_angle;
};


#endif
