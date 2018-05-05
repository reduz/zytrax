#ifndef GLOBALS_LIST_H
#define GLOBALS_LIST_H


#include "typedefs.h"


/**
 * Generic Templatized Linked List Implementation.
 * The implementation differs from the STL one because
 * a compatible preallocated linked list can be written
 * using the same API, or features such as erasing an element
 * from the iterator.
 */

template <class T>
class List {
	struct _Data;
public:

	class Comparer {

	public:

		inline bool operator()(const T& p_l, const T& p_r) const {
			return p_l < p_r;
		};
	};



	class Element {

	public:
		friend class List<T>;

		T value;
		Element* next_ptr;
		Element* prev_ptr;
		_Data *data;
	public:

		/**
		 * Get NEXT Element iterator, for constant lists.
		 */
		const Element* next() const {

			return next_ptr;
		};
		/**
		 * Get NEXT Element iterator,
		 */
		Element* next() {

			return next_ptr;
		};

		/**
		 * Get PREV Element iterator, for constant lists.
		 */
		const Element* prev() const {

			return prev_ptr;
		};
		/**
		 * Get PREV Element iterator,
		 */
		Element* prev() {

			return prev_ptr;
		};

		/**
		 * * operator, for using as *iterator, when iterators are defined on stack.
		 */
		const T& operator *() const {
			return value;
		};
		/**
		 * operator->, for using as iterator->, when iterators are defined on stack, for constant lists.
		 */
		const T* operator->() const {

			return &value;
		};
		/**
		 * * operator, for using as *iterator, when iterators are defined on stack,
		 */
		T& operator *() {
			return value;
		};
		/**
		 * operator->, for using as iterator->, when iterators are defined on stack, for constant lists.
		 */
		T* operator->() {
			return &value;
		};

		/**
		 * get the value stored in this element.
		 */
		T& get() {
			return value;
		};
		/**
		 * get the value stored in this element, for constant lists
		 */
		const T& get() const {
			return value;
		};
		/**
		 * set the value stored in this element.
		 */
		void set(const T& p_value) {
			value = (T&)p_value;
		};

		void erase() {

			data->erase(this);
		}

		Element() {
			next_ptr = 0;
			prev_ptr = 0;
			data=NULL;
		};
	};

private:

	struct _Data {

		Element* first;
		Element* last;
		int size_cache;


		bool erase(const Element* p_I) {

			if (!p_I)
				return false;
			if (p_I->data!=this)
				return false; // does not belong

			if (first==p_I) {
				first=p_I->next_ptr;
			};

			if (last==p_I)
				last=p_I->prev_ptr;

			if (p_I->prev_ptr)
				p_I->prev_ptr->next_ptr=p_I->next_ptr;

			if (p_I->next_ptr)
				p_I->next_ptr->prev_ptr=p_I->prev_ptr;

			delete  const_cast<Element*>(p_I);
			size_cache--;

			return true;
		}
	};

	_Data *_data;

public:

	/**
 	* return an const iterator to the begining of the list.
	*/
	const Element* begin() const {

		return _data?_data->first:0;
	};

	/**
 	* return an iterator to the begining of the list.
	*/
	Element* front() {
		return _data?_data->first:0;
	};

	/**
 	* return an const iterator to the last member of the list.
	*/
	const Element* back() const {

		return _data?_data->last:0;
	};

	/**
 	* return an iterator to the last member of the list.
	*/
	Element* back() {

		return _data?_data->last:0;
	};

	/**
	 * store a new element at the end of the list
	 */
	void push_back(const T& value) {

		if (!_data) {

			_data=new _Data;
			_data->first=NULL;
			_data->last=NULL;
			_data->size_cache=0;
		}

		Element* n = new Element;
		n->value = (T&)value;

		n->prev_ptr=_data->last;
		n->next_ptr=0;
		n->data=_data;

		if (_data->last) {

			_data->last->next_ptr=n;
		}

		_data->last = n;

		if (!_data->first)
			_data->first=n;

		_data->size_cache++;
	};

	void pop_back() {

		if (_data && _data->last)
			erase(_data->last);
	}

	/**
	 * store a new element at the begining of the list
	 */
	void push_front(const T& value) {

		if (!_data) {

			_data=new _Data;
			_data->first=NULL;
			_data->last=NULL;
			_data->size_cache=0;
		}

		Element* n = new Element;
		n->value = (T&)value;
		n->prev_ptr = 0;
		n->next_ptr = _data->first;
		n->data=_data;

		if (_data->first) {

			_data->first->prev_ptr=n;
		}

		_data->first = n;

		if (!_data->last)
			_data->last=n;

		_data->size_cache++;
	};

	void pop_front() {

		if (_data && _data->first)
			erase(_data->first);
	}

	/**
	 * find an element in the list,
	 */
	template<class T_v>
	Element* find(const T_v& p_val) {

		Element* it = begin();
		while (it) {
			if (it->value == p_val) return it;
			it = it->next();
		};

		return NULL;
	};

	/**
	 * erase an element in the list, by iterator pointing to it. Return true if it was found/erased.
	 */
	bool erase(const Element* p_I) {

		if (_data) {
			bool ret =  _data->erase(p_I);

			if (_data->size_cache==0) {
				delete _data;
				_data=NULL;
			}

			return ret;
		}

		return false;
	};

	/**
	 * erase the first element in the list, that contains value
	 */
	bool erase(const T& value) {

		Element* I = find(value);
		return erase(I);
	};

	/**
	 * return wether the list is empty
	 */
	bool empty() const {

		return (!_data || !_data->size_cache);
	}

	/**
	 * clear the list
	 */
	void clear() {

		while (begin()) {
			erase(begin());
		};
	};

	int size() const {

		return _data?_data->size_cache:0;

	}

	/**
	 * copy the list
	 */
	void operator=(const List& p_list) {

		clear();
		const Element *it=p_list.begin();
		while (it) {

			push_back( it->get() );
			it=it->next();
		}

	}

	T& operator[](int p_index) {

		if (p_index<0 || p_index>=size()) {
			T& aux=*((T*)0); //nullreturn
			ERR_FAIL_COND_V(p_index<0 || p_index>=size(),aux);
		}

		Element *I=begin();
		int c=0;
		while(I) {

			if (c==p_index) {

				return I->get();
			}
			I=I->next();
			c++;
		}

		ERR_FAIL_V( *((T*)0) );	// bug!!
	}

	const T& operator[](int p_index) const {

		if (p_index<0 || p_index>=size()) {
			T& aux=*((T*)0); //nullreturn
			ERR_FAIL_COND_V(p_index<0 || p_index>=size(),aux);
		}

		const Element *I=begin();
		int c=0;
		while(I) {

			if (c==p_index) {

				return I->get();
			}
			I=I->next();
			c++;
		}

		ERR_FAIL_V( *((T*)0) );	 // bug!
	}

	void move_before(Element* value, Element* where) {

		if (value->prev_ptr) {
			value->prev_ptr->next_ptr = value->next_ptr;
		};
		if (value->next_ptr) {
			value->next_ptr->prev_ptr = value->prev_ptr;
		};

		value->next_ptr = where;
		if (!where) {
			value->prev_ptr = _data->last;
			_data->last = value;
			return;
		};

		value->prev_ptr = where->prev_ptr;

		if (where->prev_ptr) {
			where->prev_ptr->next_ptr = value;
		} else {
			_data->first = value;
		};

		where->prev_ptr = value;
	};

	/**
	 * quick sort
	 */

	void sort() {

		Comparer c;
		sort<Comparer>(c);
	}
	template <class TC>
	void sort(const TC& comparer, Element* start = 0, Element* end = 0) {

		if (!start) start = begin();
		if (!end) end = back();

		if (start == end || start->next_ptr == end) return;

		Element *I = start->next();
		Element* pivot = start;
		Element* last = pivot;
		Element* start_next = pivot;
		Element* next = I->next();
		do {

			I = next;
			next = I->next();
			if (comparer((const T&)I->get(), (const T&)pivot->get())) {

				// insert before start_next
				move_before(I, start_next);
				start_next = I;
			} else {

				last = I;
			};
		} while (I != end);

		if (start_next != pivot) {
			sort(comparer, start_next, pivot->prev_ptr);
		};
		if (last != pivot) {
			sort(comparer, pivot->next_ptr, last);
		};
	};

	/**
	 * copy constructor for the list
	 */
	List(const List& p_list) {

		_data=NULL;
		const Element *it=p_list.begin();
		while (it) {

			push_back( it->get() );
			it=it->next();
		}

	}

	List() {
		_data=NULL;
	};
	~List() {
		clear();
		if (_data) {

			ERR_FAIL_COND(_data->size_cache);
			delete _data;
		}
	};
};

#endif
