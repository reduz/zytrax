//
// C++ Interface: tree_saver
//
// Description:
//
//
// Author: red <red@killy>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RESHAKEDTREE_SAVER_H
#define RESHAKEDTREE_SAVER_H

#include "typedefs.h"
#include "rstring.h"
#include "error_list.h"

/**
	@author red <red@killy>
*/
class TreeSaver{
protected:
	static TreeSaver* (*create_func)();
public:


	virtual void enter(String p_dir)=0;
	virtual void exit()=0;
	virtual String get_path()=0;
	virtual void goto_root()=0;

	virtual void add_int(String p_name,int p_int)=0;
	virtual void add_int_array(String p_name,const int *p_arr,int p_len)=0;
	virtual void add_float(String p_name,float p_float)=0;
	virtual void add_float_array(String p_name,const float *p_arr,int p_len)=0;
	virtual void add_string(String p_name,String p_string)=0;
	virtual void add_raw(String p_name,const unsigned char *p_raw,int p_bytes)=0;

	virtual void close()=0; /// when finished filling up the tree, close it
	
	virtual Error save(String p_fileID, String p_filename)=0;
	
	static TreeSaver* create();

	TreeSaver();
	virtual ~TreeSaver();

};

#endif
