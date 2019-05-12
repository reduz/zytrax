//
// C++ Implementation: rstring
//
// Description:
//
//
// Author: Juan Linietsky <juan@zytrax.org>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "rstring.h"
#include "error_macros.h"
#include "ucaps.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define MAX_DIGITS 6

#define UPPERCASE(m_c) (((m_c) >= 'a' && (m_c) <= 'z') ? ((m_c) - ('a' - 'A')) : (m_c))

void CharString::free_shared() {

	if (shared == NULL)
		return;

	if (shared->refcount == 1) {

		free(shared->data);
		free(shared);
	} else {

		shared->refcount--;
	}

	shared = NULL;
}
void CharString::take_shared(Shared *p_shared) {

	if (shared != NULL)
		free_shared();
	shared = p_shared;
	shared->refcount++;
}

const char *CharString::get_data() {

	if (!shared)
		return "";

	return shared->data;
}

const CharString &CharString::operator=(CharString &p_str) {

	take_shared(p_str.shared);
	return *this;
}

CharString::CharString() {

	shared = NULL;
}

CharString::CharString(const CharString &p_src) {

	CharString src = p_src;
	shared = NULL;
	take_shared(src.shared);
}

CharString::CharString(char *p_data) {
	if (p_data == NULL)
		shared = NULL;
	else {
		shared = (Shared *)malloc(sizeof(Shared));
		shared->data = p_data;
		shared->refcount = 1;
	}
}
CharString::~CharString() {

	free_shared();
}

/** STRING **/
void String::copy_on_write() {

	if (shared->refcount == 1)
		return; //no need to copy on write!

	Shared *old = shared;

	shared = NULL;

	create_shared();

	resize_shared(old->len);

	for (int i = 0; i < shared->len; i++) {

		shared->data[i] = old->data[i];
	}

	old->refcount--;
}

void String::resize_shared(int p_newsize) {

	shared->data = (CharType *)realloc(shared->data, (p_newsize + 1) * sizeof(CharType));
	shared->len = p_newsize;
	shared->data[shared->len] = 0; //append 0 at the end so it's compatible to a cstring
}

void String::create_shared(int p_length) {

	if (shared != NULL) {
		ERR_PRINT(" Shared != NULL ");
		return;
	}

	shared = new Shared;
	shared->len = p_length;
	shared->data = (CharType *)malloc(sizeof(CharType) * (p_length + 1));
	shared->data[p_length] = 0; //append 0 at the end so it's compatible to a cstring
	shared->refcount = 1;
}

void String::free_shared() {

	if (shared->refcount == 1) { //only us using it
		free(shared->data);
		delete shared;

	} else {

		shared->refcount--;
	}

	shared = NULL;
}

void String::copy_from(String &p_string) {

	if (p_string.shared == shared)
		return; //nothing to copy
	free_shared(); //if we have data, free it
	shared = p_string.shared; // copy the shared data
	shared->refcount++; // Increase refcount in shared
}

void String::copy_from(const char *p_cstr) {

	int len = 0;
	const char *ptr = p_cstr;
	while (*(ptr++) != 0)
		len++;

	if (shared != NULL)
		free_shared();

	create_shared(len);

	for (int i = 0; i < len; i++) {

		shared->data[i] = p_cstr[i];
	}
	shared->len = len;
}
void String::copy_from(const CharType *p_cstr, int p_clip_to_len) {

	int len = 0;
	const CharType *ptr = p_cstr;
	while (*(ptr++) != 0)
		len++;

	if (p_clip_to_len >= 0 && p_clip_to_len < len) {

		len = p_clip_to_len;
	}
	if (shared != NULL)
		free_shared();

	create_shared(len);

	for (int i = 0; i < len; i++) {

		shared->data[i] = p_cstr[i];
	}
	shared->len = len;
}

void String::copy_from(const CharType &p_char) {

	if (shared != NULL)
		free_shared();
	create_shared(1); //@TODO@ create in a certain size must be allowed
	shared->data[0] = p_char;
}

bool String::operator=(String p_str) {

	copy_from(p_str);
	return length() > 0; //true if not empty
}

bool String::operator==(String p_str) const {

	/* speed up comparison */
	if (shared == p_str.shared)
		return true; //no doubt
	if (shared->len != p_str.shared->len)
		return false; // no need for weird tests

	/* Compare char by char */
	for (int i = 0; i < shared->len; i++) {

		if (shared->data[i] != p_str.shared->data[i])
			return false;
	}

	return true;
}

bool String::operator!=(String p_str) const {

	return !(*this == p_str);
}

const String::CharType &String::operator[](int p_idx) const {

	static CharType errv = 0;
	if (p_idx < 0 || p_idx >= shared->len) {

		ERR_PRINT("p_idx <0 || p_idx>=shared->len");
		return errv;
	};

	return shared->data[p_idx];
}

String::CharType &String::operator[](int p_idx) {

	static CharType errv = 0;
	errv = 0; //dont change it, damnit
	if (p_idx < 0 || p_idx >= shared->len) {

		ERR_PRINT("p_idx <0 || p_idx>=shared->len");
		return errv;
	};

	copy_on_write();
	return shared->data[p_idx];
}

String String::operator+(const String &p_str) const {

	String res = *this;
	res += p_str;
	return res;
}

String String::operator+(CharType p_chr) const {

	String res = *this;
	res += p_chr;
	return res;
}

String &String::operator+=(const String &p_str) {

	if (p_str.empty()) {

		return *this;
	}

	copy_on_write(); /* DATA IS MODIFIED, COPY ON WRITE! */

	int old_len = shared->len;

	resize_shared(p_str.shared->len + shared->len);

	for (int i = 0; i < p_str.shared->len; i++) {

		shared->data[old_len + i] = p_str.shared->data[i];
	}

	return *this;
}

String &String::operator+=(const CharType *p_str) {

	*this += String(p_str);
	return *this;
}

String &String::operator+=(CharType p_char) {

	copy_on_write(); ///< DATA IS MODIFIED, COPY ON WRITE!

	resize_shared(shared->len + 1);
	shared->data[shared->len - 1] = p_char;
	return *this;
}

String &String::operator+=(const char *p_str) {

	if (p_str[0] == 0)
		return *this;

	copy_on_write(); ///< DATA IS MODIFIED, COPY ON WRITE!

	int src_len = 0;
	const char *ptr = p_str;
	while (*(ptr++) != 0)
		src_len++;

	int old_len = shared->len;

	resize_shared(src_len + shared->len);

	for (int i = 0; i < src_len; i++) {

		shared->data[old_len + i] = p_str[i];
	}

	return *this;
}

void String::operator=(const char *p_str) {

	copy_from(p_str);
}

void String::operator=(const CharType *p_str) {

	copy_from(p_str);
}

bool String::operator=(CharType p_chr) {

	copy_from(p_chr);
	return !empty();
}

bool String::operator==(const char *p_str) const {

	int len = 0;
	const char *aux = p_str;

	while (*(aux++) != 0)
		len++;

	if (len != shared->len)
		return false;

	for (int i = 0; i < len; i++)
		if (p_str[i] != shared->data[i])
			return false;

	return true;
}

bool String::operator==(const CharType *p_str) const {

	int len = 0;
	const CharType *aux = p_str;

	while (*(aux++) != 0)
		len++;

	if (len != shared->len)
		return false;

	for (int i = 0; i < len; i++)
		if (p_str[i] != shared->data[i])
			return false;

	return true;
}

bool String::operator!=(const char *p_str) const {

	return (!(*this == p_str));
}

bool String::operator!=(const CharType *p_str) const {

	return (!(*this == p_str));
}

bool String::operator<(const CharType *p_str) const {

	const CharType *this_str = c_str();
	while (true) {

		if (*p_str == 0 && *this_str == 0)
			return false; //this can't be equal, sadly
		else if (*this_str == 0)
			return true; //if this is empty, and the other one is not, then we're less.. I think?
		else if (*p_str == 0)
			return false; //otherwise the other one is smaller..
		else if (*this_str < *p_str) //more than
			return true;
		else if (*this_str > *p_str) //less than
			return false;

		this_str++;
		p_str++;
	}

	return false; //should never reach here anyway
}

bool String::operator<(const char *p_str) const {

	const CharType *this_str = c_str();
	while (true) {

		if (*p_str == 0 && *this_str == 0)
			return false; //this can't be equal, sadly
		else if (*this_str == 0)
			return true; //if this is empty, and the other one is not, then we're less.. I think?
		else if (*p_str == 0)
			return false; //otherwise the other one is smaller..
		else if (*this_str < *p_str) //more than
			return true;
		else if (*this_str > *p_str) //less than
			return false;

		this_str++;
		p_str++;
	}

	return false; //should never reach here anyway
}

bool String::operator<(String p_str) const {

	return operator<(p_str.c_str());
}

signed char String::nocasecmp_to(String p_str) const { ////strcmp like, <0 for we are less 0 for equal > 0 for we are greater

	const CharType *that_str = p_str.c_str();
	const CharType *this_str = c_str();
	while (true) {

		if (*that_str == 0 && *this_str == 0)
			return 0; //we're equal
		else if (*this_str == 0)
			return -1; //if this is empty, and the other one is not, then we're less.. I think?
		else if (*that_str == 0)
			return 1; //otherwise the other one is smaller..
		else if (UPPERCASE(*this_str) < UPPERCASE(*that_str)) //more than
			return -1;
		else if (UPPERCASE(*this_str) > UPPERCASE(*that_str)) //less than
			return 1;

		this_str++;
		that_str++;
	}

	return 0; //should never reach anyway
}

void String::erase(int p_pos, int p_chars) {

	*this = left(p_pos) + substr(p_pos + p_chars, length() - ((p_pos + p_chars)));
}

int String::get_slice_count(String p_splitter) {

	int pos = 0;
	int slices = 1;

	while ((pos = find(p_splitter, pos)) >= 0) {

		slices++;
		pos += p_splitter.length();
	}

	return slices;
}

String String::get_slice(String p_splitter, int p_slice) {

	int pos = 0;
	int prev_pos = 0;
	int slices = 1;
	if (p_slice < 0)
		return "";
	if (find(p_splitter) == -1)
		return *this;

	int i = 0;
	while (true) {

		pos = find(p_splitter, pos);
		if (pos == -1)
			pos = length(); //reached end

		int from = prev_pos;
		int to = pos;

		if (p_slice == i) {

			return substr(from, pos - from);
		}

		if (pos == length()) //reached end and no find
			break;
		pos += p_splitter.length();
		prev_pos = pos;
		i++;
	}

	return ""; //no find!
}

String String::to_upper() {

	String upper = *this;

	for (int i = 0; i < upper.size(); i++) {

		upper[i] = UPPERCASE(upper[i]);
	}

	return upper;
}

int String::length() const {

	return shared->len;
}
int String::size() const {

	return shared->len;
}

bool String::empty() const {

	return shared->len == 0;
}

const String::CharType *String::c_str() const {

	return shared->data;
}

String String::num(double p_num, int p_digits) {

	String s;
	String sd;
	/* integer part */

	bool neg = p_num < 0;
	p_num = fabs(p_num);
	int intn = (int)p_num;

	/* decimal part */

	if (p_digits > 0 || (p_digits == -1 && (int)p_num != p_num)) {

		double dec = p_num - floor(p_num);

		int digit = 0;
		if (p_digits > MAX_DIGITS)
			p_digits = MAX_DIGITS;

		int dec_int = 0;
		int dec_max = 0;

		while (true) {

			dec *= 10.0;
			dec_int = dec_int * 10 + (int)dec % 10;
			dec_max = dec_max * 10 + 9;
			digit++;

			if (p_digits == -1) {

				if (digit == MAX_DIGITS) //no point in going to infinite
					break;

				if ((dec - floor(dec)) < 1e-6)
					break;
			}

			if (digit == p_digits)
				break;
		}
		dec *= 10;
		int last = (int)dec % 10;

		if (last > 5) {
			if (dec_int == dec_max) {

				dec_int = 0;
				intn++;
			} else {

				dec_int++;
			}
		}

		String decimal;
		for (int i = 0; i < digit; i++) {

			decimal = String('0' + dec_int % 10) + decimal;
			dec_int /= 10;
		}
		sd = '.' + decimal;
	}

	if (intn == 0)

		s = "0";
	else {
		while (intn) {

			CharType num = '0' + (intn % 10);
			intn /= 10;

			s = num + s;
		}
	}

	s = s + sd;
	if (neg)
		s = "-" + s;
	return s;
}

CharString String::ascii(bool p_allow_extended) const {

	if (!length())
		return CharString();

	char *ascii = (char *)malloc(length() + 1);
	for (int i = 0; i < length(); i++) {

		int max = p_allow_extended ? 0xFF : 0x7F;
		if ((*this)[i] > max)
			ascii[i] = '?';
		else {

			unsigned char uc = (unsigned char)((*this)[i]);
			signed char *sc = (signed char *)&uc;
			ascii[i] = *sc;
		}
	}
	ascii[length()] = 0;

	return CharString(ascii);
}

static int parse_utf8_char(const char *p_utf8, unsigned int *p_ucs4, int p_left) { //return len

	int len = 0;

	/* Determine the number of characters in sequence */
	if ((*p_utf8 & 0x80) == 0)
		len = 1;
	else if ((*p_utf8 & 0xE0) == 0xC0)
		len = 2;
	else if ((*p_utf8 & 0xF0) == 0xE0)
		len = 3;
	else if ((*p_utf8 & 0xF8) == 0xF0)
		len = 4;
	else if ((*p_utf8 & 0xFC) == 0xF8)
		len = 5;
	else if ((*p_utf8 & 0xFE) == 0xFC)
		len = 6;
	else
		return -1; //invalid UTF8

	if (len > p_left)
		return -1; //not enough space

	if (len == 2 && (*p_utf8 & 0x1E) == 0)
		return -1; //reject overlong

	/* Convert the first character */

	unsigned int unichar = 0;

	if (len == 1)
		unichar = *p_utf8;
	else {

		unichar = (0xFF >> (len + 1)) & *p_utf8;
		;

		for (int i = 1; i < len; i++) {

			if ((p_utf8[i] & 0xC0) != 0x80)
				return -1; //invalid utf8
			if (unichar == 0 && i == 2 && ((p_utf8[i] & 0x7F) >> (7 - len)) == 0)
				return -1; //no overlong
			unichar = (unichar << 6) | (p_utf8[i] & 0x3F);
		}
	}

	*p_ucs4 = unichar;

	return len;
}

bool String::parse_utf8(const char *p_utf8) {

	String aux;

	int cstr_size = 0;

	while (p_utf8[cstr_size])
		cstr_size++;

	//	printf("Parsing %s\n",p_utf8);
	while (cstr_size) {

		unsigned int unichar;

		int len = parse_utf8_char(p_utf8, &unichar, cstr_size);
		if (len < 0)
			return true;

		//		printf("char %i, len %i\n",unichar,len);
		if (sizeof(wchar_t) == 2) { //windows most certainly

			if (unichar <= 0xFFFF) { //windows can't eat this

				aux += unichar;
			}

		} else {

			aux += unichar;
		}

		cstr_size -= len;
		p_utf8 += len;
	}

	*this = aux;
	return false;
}

CharString String::utf8() const {

	if (!length())
		return CharString();

	String utf8s;

	for (int i = 0; i < length(); i++) {

		CharType c = (*this)[i];

		if (c <= 0x7f) // 7 bits.
			utf8s += c;
		else if (c <= 0x7ff) { // 11 bits

			utf8s += CharType(0xc0 | ((c >> 6) & 0x1f)); // Top 5 bits.
			utf8s += CharType(0x80 | (c & 0x3f)); // Bottom 6 bits.
		} else if (c <= 0xffff) { // 16 bits

			utf8s += CharType(0xe0 | ((c >> 12) & 0x0f)); // Top 4 bits.
			utf8s += CharType(0x80 | ((c >> 6) & 0x3f)); // Middle 6 bits.
			utf8s += CharType(0x80 | (c & 0x3f)); // Bottom 6 bits.
		} else if (c <= 0x001fffff) { // 21 bits

			utf8s += CharType(0xf0 | ((c >> 18) & 0x07)); // Top 3 bits.
			utf8s += CharType(0x80 | ((c >> 12) & 0x3f)); // Upper middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 6) & 0x3f)); // Lower middle 6 bits.
			utf8s += CharType(0x80 | (c & 0x3f)); // Bottom 6 bits.
		} else if (c <= 0x03ffffff) { // 26 bits

			utf8s += CharType(0xf8 | ((c >> 24) & 0x03)); // Top 2 bits.
			utf8s += CharType(0x80 | ((c >> 18) & 0x3f)); // Upper middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 12) & 0x3f)); // middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 6) & 0x3f)); // Lower middle 6 bits.
			utf8s += CharType(0x80 | (c & 0x3f)); // Bottom 6 bits.
		} else if (c <= 0x7fffffff) { // 31 bits

			utf8s += CharType(0xfc | ((c >> 30) & 0x01)); // Top 1 bit.
			utf8s += CharType(0x80 | ((c >> 24) & 0x3f)); // Upper upper middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 18) & 0x3f)); // Lower upper middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 12) & 0x3f)); // Upper lower middle 6 bits.
			utf8s += CharType(0x80 | ((c >> 6) & 0x3f)); // Lower lower middle 6 bits.
			utf8s += CharType(0x80 | (c & 0x3f)); // Bottom 6 bits.
		}
	}

	return utf8s.ascii(true); //allow extended
}

String::String(CharType p_char) {

	shared = NULL;
	copy_from(p_char);
}

String::String(const String &p_string) {

	shared = NULL;
	create_shared();
	String &str = (String &)p_string; /* remove Const-ness */
	copy_from(str);
}

String::String(const char *p_str) {

	shared = NULL;
	copy_from(p_str);
}
String::String(const CharType *p_str, int p_clip_to_len) {

	shared = NULL;
	copy_from(p_str, p_clip_to_len);
}

String::String() {

	shared = NULL;
	create_shared();
}

int String::to_int() const {

	if (length() == 0)
		return 0;

	int to = (find(".") >= 0) ? find(".") : length();

	int integer = 0;

	for (int i = 0; i < to; i++) {

		CharType c = operator[](i);
		if (c >= '0' && c <= '9') {

			integer *= 10;
			integer += c - '0';
		}
	}

	return integer;
}

double String::to_double() const {

	if (length() == 0)
		return 0;

	int dot = find(".");

	if (dot < 0)
		dot = length();

	int integer = to_int();

	double decimal = 0;

	if (dot < length()) { //has decimal part?

		double multiplier = 0.1;

		for (int i = (dot + 1); i < length(); i++) {

			CharType c = operator[](i);

			if (c >= '0' && c <= '9') {

				decimal += (double)(c - '0') * multiplier;
				multiplier *= 0.1;
			}
		}
	}

	return (double)integer + decimal;
}

String::~String() {

	free_shared();
}
bool operator==(const char *p_chr, const String &p_str) {

	return p_str == p_chr;
}

String operator+(const char *p_chr, const String &p_str) {

	String tmp = p_chr;
	tmp += p_str;
	return tmp;
}
String operator+(String::CharType p_chr, const String &p_str) {

	String tmp(p_chr);
	tmp += p_str;
	return tmp;
}

void String::insert(int p_at_pos, String p_string) {

	if (p_at_pos < 0)
		return;

	if (p_at_pos > length())
		p_at_pos = length();

	String pre;
	if (p_at_pos > 0)
		pre = substr(0, p_at_pos);

	String post;
	if (p_at_pos < length())
		post = substr(p_at_pos, length() - p_at_pos);

	*this = pre + p_string + post;
}
String String::substr(int p_from, int p_chars) const {

	if (p_from < 0 || p_from >= length() || p_chars <= 0)
		return "";

	if ((p_from + p_chars) > length()) {

		p_chars = length() - p_from;
	}

	return String(&shared->data[p_from], p_chars);
}
int String::find(String p_str, int p_from) const {

	if (p_from < 0)
		return -1;

	int src_len = p_str.length();

	if (src_len == 0 || length() == 0)
		return -1; //wont find anything!

	for (int i = p_from; i <= (length() - src_len); i++) {

		bool found = true;
		for (int j = 0; j < src_len; j++) {

			int read_pos = i + j;

			if (read_pos >= length()) {

				ERR_PRINT("read_pos>=length()");
				return -1;
			};

			if (shared->data[read_pos] != p_str[j]) {
				found = false;
				break;
			}
		}

		if (found)
			return i;
	}

	return -1;
}

int String::find_last(String p_str) const {

	int idx = find(p_str);

	if (idx == -1)
		return -1;

	while (true) {

		int res = find(p_str, idx + 1);
		if (res == -1)
			return idx;
		idx = res;
	}
}
int String::findn(String p_str, int p_from) const {

	if (p_from < 0)
		return -1;

	int src_len = p_str.length();

	if (src_len == 0 || length() == 0)
		return -1; //wont find anything!

	for (int i = p_from; i <= (length() - src_len); i++) {

		bool found = true;
		for (int j = 0; j < src_len; j++) {

			int read_pos = i + j;

			if (read_pos >= length()) {

				ERR_PRINT("read_pos>=length()");
				return -1;
			};

			CharType src = shared->data[read_pos];
			CharType dst = p_str[j];

			if (src >= 'a' && src <= 'z')
				src -= 'a' - 'A';
			if (dst >= 'a' && dst <= 'z')
				dst -= 'a' - 'A';

			if (src != dst) {
				found = false;
				break;
			}
		}

		if (found)
			return i;
	}

	return -1;
}

void String::replace(String p_key, String p_with) {

	String new_string;
	int search_from = 0;
	int result = 0;

	while ((result = find(p_key, search_from)) >= 0) {

		new_string += substr(search_from, result - search_from);
		new_string += p_with;
		search_from = result + p_key.length();
	}

	new_string += substr(search_from, length() - search_from);

	*this = new_string;
}

String String::left(int p_chars) {

	if (p_chars <= 0)
		return "";

	if (p_chars >= length())
		return *this;

	return substr(0, p_chars);
}

String String::right(int p_chars) {

	int from = (int)length() - p_chars;
	if (from < 0)
		return "";
	int len = p_chars;

	return substr(from, p_chars);
}

String String::strip_edges() {

	int beg = 0, end = length();

	for (int i = 0; i < length(); i++) {

		if (operator[](i) <= 32)
			beg++;
		else
			break;
	}

	for (int i = (int)(length() - 1); i >= 0; i--) {

		if (operator[](i) <= 32)
			end++;
		else
			break;
	}

	return substr(beg, end - beg);
}

String String::get_extension() const {

	int pos = find_last(".");
	if (pos < 0 || pos < MAX(find_last("/"), find_last("\\")))
		return "";

	return substr(pos + 1, length());
}

String String::to_upper() const {

	String upper = *this;

	for (int i = 0; i < upper.size(); i++) {

		const CharType s = upper[i];
		const CharType t = _find_upper(s);
		if (s != t) // avoid copy on write
			upper[i] = t;
	}

	return upper;
}

String String::to_lower() const {

	String lower = *this;

	for (int i = 0; i < lower.size(); i++) {

		const CharType s = lower[i];
		const CharType t = _find_lower(s);
		if (s != t) // avoid copy on write
			lower[i] = t;
	}

	return lower;
}
