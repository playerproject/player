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

#include "playercommon.h"
#include "player.h"


/** @addtogroup player_classes */
/** @{ */

/** Class for loading/saving config file.
 */
class ConfigFile
{
  // Standard constructors/destructors
  public: ConfigFile();
  public: ~ConfigFile();

  // Load config from file
  public: bool Load(const char *filename);

  // Save config back into file
  // Set filename to NULL to save back into the original file
  public: bool Save(const char *filename);

  // Check for unused fields and print warnings
  public: bool WarnUnused();

  // Read a string
  public: const char *ReadString(int section, 
                                 const char *name, 
                                 const char *value);

  // Write a string
  public: void WriteString(int section, 
                           const char *name, 
                           const char *value);

  // Read an integer 
  public: int ReadInt(int section, 
                      const char *name, 
                      int value);

  // Write an integer
  public: void WriteInt(int section, 
                        const char *name, 
                        int value);

  // Read a float 
  public: double ReadFloat(int section, 
                           const char *name, 
                           double value);

  // Write a float
  public: void WriteFloat(int section, 
                          const char *name, 
                          double value);

  // Read a length (includes unit conversion)
  public: double ReadLength(int section, 
                            const char *name, 
                            double value);

  // Write a length (includes units conversion)
  public: void WriteLength(int section, 
                           const char *name, 
                           double value);
  
  // Read an angle (includes unit conversion)
  public: double ReadAngle(int section, const char *name, double value);

  // Read a color (includes text to RGB conversion)
  public: uint32_t ReadColor(int section, 
                             const char *name, 
                             uint32_t value);

  // Read a file name.  Always returns an absolute path.  If the
  // filename is entered as a relative path, we prepend the config
  // file's path to it.
  public: const char *ReadFilename(int section, 
                                   const char *name, 
                                   const char *value);

  // Get the number of values in a tuple
  public: int GetTupleCount(int section, const char *name);

  // Read a string from a tuple
  public: const char *ReadTupleString(int section, 
                                      const char *name,
                                      int index, 
                                      const char *value);
  
  // Write a string to a tuple
  public: void WriteTupleString(int section, 
                                const char *name,
                                int index, 
                                const char *value);
  
  // Read a int from a tuple
  public: int ReadTupleInt(int section, 
                           const char *name,
                           int index, 
                           int value);

  // Write a int to a tuple
  public: void WriteTupleInt(int section, 
                             const char *name,
                             int index, 
                             int value);
  
  // Read a float from a tuple
  public: double ReadTupleFloat(int section, 
                                const char *name,
                                int index, 
                                double value);

  // Write a float to a tuple
  public: void WriteTupleFloat(int section, 
                               const char *name,
                               int index, 
                               double value);

  // Read a length from a tuple (includes units conversion)
  public: double ReadTupleLength(int section, 
                                 const char *name,
                                 int index, 
                                 double value);

  // Write a to a tuple length (includes units conversion)
  public: void WriteTupleLength(int section, 
                                const char *name,
                                int index, 
                                double value);

  // Read an angle form a tuple (includes units conversion)
  public: double ReadTupleAngle(int section, 
                                const char *name,
                                int index, 
                                double value);

  // Write an angle to a tuple (includes units conversion)
  public: void WriteTupleAngle(int section, 
                               const char *name,
                               int index, 
                               double value);

  // Read a color (includes text to RGB conversion)
  public: uint32_t ReadTupleColor(int section, 
                                  const char *name,
                                  int index, 
                                  uint32_t value); 

  /// Parse the "devices" option in the given section.  On success,
  /// returns the number of device ids found; on error, returns -1.
  /// @arg ids will point to a malloc()ed list of the parsed ids
  /// (which the caller should free()), in the order they were given.
  public: int ParseDeviceIds(int section, player_device_id_t** ids);
  
  /// Given a list of ids (e.g., one returned by ParseDeviceIds()) of length
  /// num_ids, if there exists an i'th id with the given code, fills in id
  /// appropriately, and returns 0.
  ///
  /// "Consumes" the selected id in the list, by setting the code to 0.  Thus,
  /// after calling ReadDeviceId() for each supported interface, you can call
  /// UnusedIds() to determine whether the user gave any extra interfaces.
  ///
  /// Returns -1 if no such id can be found.
  public: int ReadDeviceId(player_device_id_t* id, player_device_id_t* ids,
                           int num_ids, int code, int i);

  /// Given a list of ids (e.g., one returned by ParseDeviceIds()) of length
  /// num_ids, tells whether there remain any "unused" ids.  An id is unused
  /// if its code is nonzero.
  public:  int UnusedIds(int section, player_device_id_t* ids, int num_ids);

  /// @brief Read a device id.
  ///
  /// Reads a device id from the named field of the given section.
  /// The returned id will match the given code, index and key values.
  //
  /// @param id[out] ID field to be filled in.
  /// @param section[in] File section.
  /// @param name[in] Field name.
  /// @param code[in] Interface type code (use 0 to match all interface types).
  /// @param index[in] Tuple index (use -1 to match all indices).
  /// @param key[in] Device key value (use NULL to match all key vales).
  /// @return Non-zero on error.
  public: int ReadDeviceId(player_device_id_t *id, int section, const char *name,
                           int code, int index, const char *key);

  // Get the number of sections.
  public: int GetSectionCount();

  // Get a section (returns the section type value)
  public: const char *GetSectionType(int section);

  // Lookup a section number by type name
  // Returns -1 if there is section with this type
  public: int LookupSection(const char *type);
  
  // Get a section's parent section.
  // Returns -1 if there is no parent.
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

  // Token types.
  public: enum
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

  // Name of the file we loaded
  public: char *filename;

  // Conversion units
  private: double unit_length;
  private: double unit_angle;
};

/** @} */

#endif
