//
// C++ Implementation: tree_loader
//
// Description:
//
//
// Author: red <red@killy>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "tree_loader.h"

TreeLoader* (*TreeLoader::create_func)()=NULL;


TreeLoader* TreeLoader::create() {

	if (create_func)
		return create_func();
	else 
		return NULL;
	
}


TreeLoader::TreeLoader()
{
}


TreeLoader::~TreeLoader()
{
}


