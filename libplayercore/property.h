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
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

#ifndef __PROPERTY_H
#define __PROPERTY_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCORE_EXPORT
  #elif defined (playercore_EXPORTS)
    #define PLAYERCORE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCORE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCORE_EXPORT
#endif

class ConfigFile;
class Driver;

/// Property base class
class PLAYERCORE_EXPORT Property
{
	public:
		Property (void);
		Property (const char *newKey, bool readOnly);
		virtual ~Property (void);

		// Accessor functions
		virtual const char* GetKey (void) const		{ return key; }
		virtual void SetKey (const char *newKey);
		virtual void GetValueToMessage (void *data) const = 0;
		virtual void SetValueFromMessage (const void *data) = 0;

		virtual const bool KeyIsEqual (const char *rhs);

		// Config file read method
		virtual bool ReadConfig (ConfigFile *cf, int section) = 0;

	protected:
		char *key;			// Key for this property
		bool readonly;		// true if this property is read-only
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Boolean property class
class PLAYERCORE_EXPORT BoolProperty : public Property
{
	public:
		BoolProperty (const char *newKey, bool newValue, bool readOnly);
		/** Constructor that registers the property with a driver as well while it is constructed */
		BoolProperty (const char *newKey, bool newValue, bool readOnly, Driver * driver, ConfigFile*cf, int section);

		bool GetValue (void) const		{ return value; }
		void SetValue (bool newValue);
		void GetValueToMessage (void *data) const;
		void SetValueFromMessage (const void *data);

		// Config file read method
		virtual bool ReadConfig (ConfigFile *cf, int section);

		// Operators
		operator bool (void)			{ return value; }
		const BoolProperty& operator= (const BoolProperty &rhs);
		bool operator= (bool rhs);

	private:
		bool value;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Integer property class
class PLAYERCORE_EXPORT IntProperty : public Property
{
	public:
		IntProperty (const char *newKey, int newValue, bool readOnly);
		/** Constructor that registers the property with a driver as well while it is constructed */
		IntProperty (const char *newKey, int newValue, bool readOnly, Driver * driver, ConfigFile*cf, int section);

		int GetValue (void) const			{ return value; }
		void SetValue (int newValue);
		void GetValueToMessage (void *data) const;
		void SetValueFromMessage (const void *data);

		// Config file read method
		virtual bool ReadConfig (ConfigFile *cf, int section);

		// Operators
		operator int (void)				{ return value; }
		const IntProperty& operator= (const IntProperty &rhs);
		int operator= (int rhs);

	private:
		int value;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Double property class
class PLAYERCORE_EXPORT DoubleProperty : public Property
{
	public:
		DoubleProperty (const char *newKey, double newValue, bool readOnly);
		/** Constructor that registers the property with a driver as well while it is constructed */
		DoubleProperty (const char *newKey, double newValue, bool readOnly, Driver * driver, ConfigFile*cf, int section);

		double GetValue (void) const		{ return value; }
		void SetValue (double newValue);
		void GetValueToMessage (void *data) const;
		void SetValueFromMessage (const void *data);

		// Config file read method
		virtual bool ReadConfig (ConfigFile *cf, int section);

		// Operators
		operator double (void)				{ return value; }
		const DoubleProperty& operator= (const DoubleProperty &rhs);
		double operator= (double rhs);

	private:
		double value;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// String Property Class
class PLAYERCORE_EXPORT StringProperty : public Property
{
	public:
		StringProperty (const char *newKey, const char *newValue, bool readOnly);
		/** Constructor that registers the property with a driver as well while it is constructed */
		StringProperty (const char *newKey, const char *newValue, bool readOnly, Driver * driver, ConfigFile*cf, int section);
		~StringProperty (void);

		const char* GetValue (void) const	{ return value; }
		void SetValue (const char *newValue);
		void GetValueToMessage (void *data) const;
		void SetValueFromMessage (const void *data);

		// Config file read method
		virtual bool ReadConfig (ConfigFile *cf, int section);

		// Operators
		operator const char* (void)				{ return value; }
		const StringProperty& operator= (const StringProperty &rhs);
		const char* operator= (const char* rhs);

	private:
		char *value;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Property node structure
typedef struct PropertyNode
{
	char *key;
	Property *property;
	struct PropertyNode *next;
} PropertyNode;

/// Property bag class: stores registered properties
class PLAYERCORE_EXPORT PropertyBag
{
	public:
		PropertyBag (void);
		~PropertyBag (void);

		bool AddProperty (const char *key, Property *property);
		Property* GetProperty (const char *key);

	private:
		PropertyNode *firstProperty;
};

#endif // __PROPERTY_H
