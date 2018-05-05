
//
// C++ Interface: value_stream
//
// Description:
//
//
// Author: Juan Linietsky <coding@reduz.com.ar>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef VALUE_STREAM_H
#define VALUE_STREAM_H

#include "error_macros.h"
#include "typedefs.h"
#include "vector.h"

template <class T, class V>
class ValueStream {
private:
	struct Value {

		T pos;
		V val;
	};

	Vector<Value> stream;

	_FORCE_INLINE_ int find_internal(T p_pos, bool &p_exact) const;

public:
	int insert(T p_pos, V p_value); /* Insert, and return the position at it was inserted */
	int find_exact(T p_pos) const; /* return INVALID_STREAM_INDEX if pos is not exact */
	int find(T p_pos) const; /* get index to pos previous or equal to value, if nothing less than it, return -1 */

	_FORCE_INLINE_ const V &operator[](int p_idx) const; /* return a const reference to     const V& get_index_value(int p_idx);  return a const reference to null if index is invalid! */
	_FORCE_INLINE_ V &operator[](int p_idx); /* return a reference to     const V& get_index_value(int p_idx); return a const reference to null if index is invalid! */
	const T &get_pos(int p_idx) const; /* return a const reference to     const T& get_pos(int p_idx);  return a const reference to null if index is invalid! */

	_FORCE_INLINE_ int size() const;

	void erase(int p_index);
	void clear();
};

template <class T, class V>
int ValueStream<T, V>::find_internal(T p_pos, bool &p_exact) const {

	/* The core of this class, the binary search */
	p_exact = false;

	if (stream.empty())
		return -1;

	int low = 0;
	int high = stream.size() - 1;
	int middle;
	const Value *a = &stream[0];

	while (low <= high) {
		middle = (low + high) / 2;

		if (p_pos == a[middle].pos) { //match
			p_exact = true;
			return middle;
		} else if (p_pos < a[middle].pos)
			high = middle - 1; //search low end of array
		else
			low = middle + 1; //search high end of array
	}

	if (a[middle].pos > p_pos)
		middle--;
	return middle;
}

template <class T, class V>
int ValueStream<T, V>::find(T p_pos) const {

	bool _e;
	return find_internal(p_pos, _e);
}

template <class T, class V>
int ValueStream<T, V>::insert(T p_pos, V p_value) {

	Value new_v;
	new_v.pos = p_pos;
	new_v.val = p_value;

	bool exact;
	int pos = find_internal(p_pos, exact);

	if (!exact) { /* no exact position found, make room */
		pos++;
		if (pos == stream.size())
			stream.push_back(new_v); //it's at the end, just pushit back
		else
			stream.insert(pos, new_v);
	} else {

		stream[pos] = new_v; /* Overwrite, sine exact position */
	}

	return pos;
}

template <class T, class V>
const V &ValueStream<T, V>::operator[](int p_index) const {

	ERR_FAIL_INDEX_V(p_index, stream.size(), *((V *)(NULL)));
	return stream[p_index].val;
}

template <class T, class V>
V &ValueStream<T, V>::operator[](int p_index) {

	ERR_FAIL_INDEX_V(p_index, stream.size(), *((V *)(NULL)));
	return stream[p_index].val;
}

template <class T, class V>
const T &ValueStream<T, V>::get_pos(int p_index) const {

	ERR_FAIL_INDEX_V(p_index, stream.size(), *((T *)(NULL)));
	return stream[p_index].pos;
}

template <class T, class V>
int ValueStream<T, V>::find_exact(T p_pos) const {

	bool exact;
	int pos = find_internal(p_pos, exact);
	if (!exact)
		return -1;
	return pos;
}

template <class T, class V>
int ValueStream<T, V>::size() const {

	return stream.size();
}

template <class T, class V>
void ValueStream<T, V>::erase(int p_index) {

	ERR_FAIL_INDEX(p_index, stream.size());
	stream.remove(p_index);
}

template <class T, class V>
void ValueStream<T, V>::clear() {

	stream.clear();
}
#endif
