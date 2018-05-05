//
// C++ Implementation: tree_saver
//
// Description:
//
//
// Author: red <red@killy>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "tree_saver.h"

TreeSaver* (*TreeSaver::create_func)()=NULL;

TreeSaver* TreeSaver::create() {

	if (create_func)
		return create_func();
			
	return NULL;
}

TreeSaver::TreeSaver()
{
}


TreeSaver::~TreeSaver()
{
}


