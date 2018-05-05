//
// C++ Interface: vector
//
// Description: 
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef VECTOR_H
#define VECTOR_H

#include "error_list.h"
#include <stdlib.h>
/**
 * @class Vector
 * @author Juan Linietsky
 * Vector container. Regular Vector Container. Use with care and for smaller arrays when possible. Use DVector for large arrays.
*/

#include "error_macros.h"

template<class T>
class Vector {

	T* ptr;
	int element_count;

	void copy_from(const Vector& p_from);

public:


	void clear() { resize(0); }
	inline int size() const;
	inline bool empty() const;
	Error resize(int p_size);
	bool push_back(T p_elem);
	
	void remove(int p_index);

	template <class T_val>
	int find(T_val& p_val) const;

	void set(int p_index,T p_elem);
	T get(int p_index) const;

	inline T& operator[](int p_index) {

		if (p_index<0 || p_index>=element_count) {
			T& aux=*((T*)0); //nullreturn
			ERR_FAIL_COND_V(p_index<0 || p_index>=element_count,aux);
		}

		return ptr[p_index];
	}

	inline const T& operator[](int p_index) const {

		if (p_index<0 || p_index>=element_count) {
			const T& aux=*((T*)0); //nullreturn
			ERR_FAIL_COND_V(p_index<0 || p_index>=element_count,aux);
		}

		return ptr[p_index];
	}
	
	Error insert(int p_pos,const T& p_val);

	void operator=(const Vector& p_from);
	Vector(const Vector& p_from);

	Vector();
	~Vector();

};

template<class T>
inline bool Vector<T>::empty() const {

	return element_count==0;
}

template<class T>
inline int Vector<T>::size() const {

	return element_count;
}

template<class T> template<class T_val>
int Vector<T>::find(T_val& p_val) const {

	int ret = -1;
	if (element_count == 0) return ret;
	for (int i=0; i<element_count; i++) {
		if (operator[](i) == p_val) {
			ret = i;
			break;
		};
	};

	return ret;
};

template<class T>
Error Vector<T>::resize(int p_size) {

	ERR_FAIL_COND_V(p_size<0,ERR_INVALID_PARAMETER);

	if (p_size>element_count) {
		//create elements
		//int new_elems=p_size-element_count;

		if (element_count==0) {

			ptr = (T*)malloc(p_size*sizeof(T));
			ERR_FAIL_COND_V( !ptr ,ERR_OUT_OF_MEMORY);

		} else {
			
			T *ptrnew = (T*)realloc(ptr,p_size*sizeof(T));
			ERR_FAIL_COND_V( !ptrnew ,ERR_OUT_OF_MEMORY);		
			ptr=ptrnew;	
		}

		for (int i=element_count;i<p_size;i++) {
			
			new(&ptr[i]) T;
		}

		element_count=p_size;

	} else if (p_size<element_count) {

		for (int i=p_size;i<element_count;i++) {

			T* t = &ptr[i];
			t->~T();

		}

		if (p_size>0) {

			T *ptrnew = (T*)realloc(ptr,p_size*sizeof(T));
			ERR_FAIL_COND_V( !ptrnew ,ERR_OUT_OF_MEMORY);		
			ptr=ptrnew;	
		} else {
			free( ptr );
		}

		element_count=p_size;
	}

	return OK;
}




template<class T>
void Vector<T>::set(int p_index,T p_elem) {

	operator[](p_index)=p_elem;
}

template<class T>
T Vector<T>::get(int p_index) const {

	return operator[](p_index);
}

template<class T>
bool Vector<T>::push_back(T p_elem) {


	ERR_FAIL_COND_V( resize(element_count+1), true )
	set(element_count-1,p_elem);

	return false;
}


template<class T>
void Vector<T>::remove(int p_index) {

	ERR_FAIL_INDEX(p_index, element_count);
	for (int i=p_index; i<element_count-1; i++) {

		set(i, get(i+1));
	};

	resize(element_count-1);
};

template<class T>
void Vector<T>::copy_from(const Vector& p_from) {

	resize(p_from.size());
	for (int i=0;i<p_from.size();i++) {

		set( i, p_from.get(i) );
	}
}

template<class T>
void Vector<T>::operator=(const Vector& p_from) {

	copy_from(p_from);

}


template<class T>
Error Vector<T>::insert(int p_pos,const T& p_val) {
	
	ERR_FAIL_INDEX_V(p_pos,size()+1,ERR_INVALID_PARAMETER);
	resize(size()+1);
	for (int i=(size()-1);i>p_pos;i--)
		set( i, get(i-1) );
	set( p_pos, p_val );
	
	return OK;
}

template<class T>
Vector<T>::Vector(const Vector& p_from) {

	ptr=NULL;
	element_count=0;
	copy_from( p_from );

}

template<class T>
Vector<T>::Vector() {

	ptr=NULL;
	element_count=0;
}


template<class T>
Vector<T>::~Vector() {

	resize(0);

}


#endif
