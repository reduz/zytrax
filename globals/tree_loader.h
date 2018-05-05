//
// C++ Interface: tree_loader
//
// Description:
//
//
// Author: red <red@killy>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RESHAKEDTREE_LOADER_H
#define RESHAKEDTREE_LOADER_H

#include "rstring.h"
//#include "base/file.h"
#include "error_list.h"
#include <stddef.h>

/**
 * TreeLoader.
 * This is a generic accessor for loading data from a tree-organized data structure or file.
 * It helps to share/move/transfer data in an organized format or protocol.
 * Right now, XML and Binary formats can be used, but filesystem backends could also exist.
 * Think of it as a simplified version of an XML-like tree API.
 * To avoid confusion, this is meant for data storage, not real filesystem directories.
 * NOTE: Be careful with the length of the variables inside each subpath, most implementations
 * limit the length. (MAX_ENTRY_NAME_LEN)
 * @TODO - punto, mass rename "child" to "dir"
 * @MAYBE - Add a DOUBLE type in the future, for storing stuff like matrices.
 */


class TreeLoader{
protected:
	static TreeLoader* (*create_func)();

public:

	enum VarType {
		VAR_INT, /// 32-bits Integer type
		VAR_FLOAT, /// 32 bits float type
		VAR_STRING, /// string type
		VAR_INT_ARRAY, /// string array type, any size
		VAR_FLOAT_ARRAY, /// 32 bits floating point array type
		VAR_STRING_ARRAY, /// array of strings type
		VAR_RAW, /// raw data (bytes) for storing anything else
		VAR_NONE, /// if the var doesnt exist, used also as error status
	};

	virtual bool enter(String p_dir)=0; /// snter a sub-folder
	virtual bool enter_by_index(int p_index)=0; /// enter by sub-folder index
	virtual void exit()=0; /// exit a sub-folder
	virtual String get_path()=0; /// get current path
	virtual void goto_root()=0; /// go back to root path

	virtual int get_int(String p_name)=0; /// get an integer variable by name
	/**
	 * Get an integer array
	 * @param p_name name of the array variable
	 * @param p_arr pointer to an int* array that will receive the data
	 * @param p_from from position index, by default 0
	 * @param p_len amount of positions to retrieve, by default all (-1)
	 */
	virtual void get_int_array(String p_name,int *p_arr,int p_from=0,int p_len=-1)=0; /// get integer array
	virtual int get_int_array_len(String p_name)=0; ///get length of integer array
	virtual float get_float(String p_name)=0; ///get 32 bits float variable
	/**
	 * Get floating point array
	 * @param p_name name of the array variable
	 * @param p_arr pointer to float* flating point array that will receive the data
	 * @param p_from from position index, by default 0
	 * @param p_len amount of positions to retrieve, by default all (-1)
	 */
	virtual void get_float_array(String p_name,float*,int p_from=0,int p_len=-1)=0; /// get floating point array
	virtual int get_float_array_len(String p_name)=0; ///get length of floating point array
	virtual String get_string(String p_name)=0; /// get string variable
	/**
	 * Get array of string variables
	 * @param p_name name of the array variable
	 * @param p_arr pointer to String* array that will receive the data
	 * @param p_from from position index, by default 0
	 * @param p_len  amount of positions to retrieve, by default all (-1)
	 */
	virtual void get_string_array(String p_name,String*,int p_from=0,int p_len=-1)=0; ///get array of strings
	virtual int get_string_array_len(String p_name)=0; ///get length of string array
	
	/**
	 * Get buffer of raw data (bytes)
	 * @param p_name name of the buffer variable
	 * @param p_raw pointer to a buffer that will receive the data
	 * @param p_from from byte index, default 0
	 * @param p_len amount of bytes to retrieve, by default al (-1)
	 */
	virtual void get_raw(String p_name,unsigned char *p_raw,int p_from=0,int p_len=-1)=0; ///get raw data buffer
	virtual int get_raw_len(String p_name)=0; ///get length of raw data buffer, in bytes

	virtual int get_var_count()=0; ///get amount of variables in current path
	virtual String get_var_name(int i)=0; ///get name of a variable, by index, in current path

	virtual int get_child_count()=0; ///get amount of child sub-paths in this path
	virtual String get_child_name(int i)=0; ///get name of child sub-path, by index
	virtual bool is_child(String p_dir)=0; /// return true if this path contains a current path

	virtual bool is_var(String p_var)=0; /// return true if this path contains a given variable
	virtual VarType get_var_type(String p_var)=0; /// return the type of a variable in the current path

	
	/**
	 * Method for opening a tree in a custom file format. return the proper error if failed.
	 * This method is not implemented for all kind of trees, only those which are stored in files.
	 * If so, then only upon opening, all the other methods in this class will work, otherwise they
	 * will return the default value with an error condition notified.
	 * @param p_file file where the tree is located
	 * @param p_header identifier for the tree, to validate
	 * @param p_custom A custom fileaccess can be used for opening the file.
	 * @return OK if success, a specific error if failed
	 */
	 
	virtual Error open(String p_fileID, String p_filename)=0;
	virtual void close()=0; /// close the file. Also, if open, file is always closed when the object is deleted.
	
	static TreeLoader* create();
	
	TreeLoader();
	virtual ~TreeLoader();

};

#endif
