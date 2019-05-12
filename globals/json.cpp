/*
Copyright (c) 2015 Johannes Häggqvist

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifdef JZON_DLL
#if defined _WIN32 || defined __CYGWIN__
#define JZON_API __declspec(dllexport)
#define JZON_STL_EXTERN
#endif
#endif

#include "json.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <stack>

namespace JSON {
namespace {
inline bool isWhitespace(char c) {
	return (c == '\n' || c == ' ' || c == '\t' || c == '\r' || c == '\f');
}

const char charsUnescaped[] = { '\\', '/', '\"', '\n', '\t', '\b', '\f', '\r' };
const char *charsEscaped[] = { "\\\\", "\\/", "\\\"", "\\n", "\\t", "\\b", "\\f", "\\r" };
const unsigned int numEscapeChars = 8;
const char nullUnescaped = '\0';
const char *nullEscaped = "\0\0";
const char *getEscaped(const char c) {
	for (unsigned int i = 0; i < numEscapeChars; ++i) {
		const char &ue = charsUnescaped[i];

		if (c == ue) {
			return charsEscaped[i];
		}
	}
	return nullEscaped;
}
char getUnescaped(const char c1, const char c2) {
	for (unsigned int i = 0; i < numEscapeChars; ++i) {
		const char *e = charsEscaped[i];

		if (c1 == e[0] && c2 == e[1]) {
			return charsUnescaped[i];
		}
	}
	return nullUnescaped;
}
} // namespace

Node::Node() :
		data(NULL) {
}
Node::Node(Type type) :
		data(NULL) {
	if (type != T_INVALID) {
		data = new Data(type);
	}
}
Node::Node(const Node &other) :
		data(other.data) {
	if (data != NULL) {
		data->addRef();
	}
}
Node::Node(Type type, const std::string &value) :
		data(new Data(T_NULL)) {
	set(type, value);
}
Node::Node(const std::string &value) :
		data(new Data(T_STRING)) {
	set(value);
}
Node::Node(const char *value) :
		data(new Data(T_STRING)) {
	set(value);
}
Node::Node(int value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(unsigned int value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(long long value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(unsigned long long value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(float value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(double value) :
		data(new Data(T_NUMBER)) {
	set(value);
}
Node::Node(bool value) :
		data(new Data(T_BOOL)) {
	set(value);
}
Node::~Node() {
	if (data != NULL && data->release()) {
		delete data;
		data = NULL;
	}
}

void Node::detach() {
	if (data != NULL && data->refCount > 1) {
		Data *newData = new Data(*data);
		if (data->release()) {
			delete data;
		}
		data = newData;
	}
}

std::string Node::toString(const std::string &def) const {
	if (isValue()) {
		if (isNull()) {
			return std::string("null");
		} else {
			return data->valueStr;
		}
	} else {
		return def;
	}
}
#define GET_NUMBER(T)                           \
	if (isNumber()) {                           \
		std::stringstream sstr(data->valueStr); \
		T val;                                  \
		sstr >> val;                            \
		return val;                             \
	} else {                                    \
		return def;                             \
	}
int Node::toInt(int def) const {
	GET_NUMBER(int)
}
float Node::toFloat(float def) const {
	GET_NUMBER(float)
}
double Node::toDouble(double def) const {
	GET_NUMBER(double)
}
#undef GET_NUMBER
bool Node::toBool(bool def) const {
	if (isBool()) {
		return (data->valueStr == "true");
	} else {
		return def;
	}
}

void Node::setNull() {
	if (isValue()) {
		detach();
		data->type = T_NULL;
		data->valueStr.clear();
	}
}
void Node::set(Type type, const std::string &value) {
	if (isValue() && (type == T_NULL || type == T_STRING || type == T_NUMBER || type == T_BOOL)) {
		detach();
		data->type = type;
		if (type == T_STRING) {
			data->valueStr = unescapeString(value);
		} else {
			data->valueStr = value;
		}
	}
}
void Node::set(const std::string &value) {
	if (isValue()) {
		detach();
		data->type = T_STRING;
		data->valueStr = unescapeString(value);
	}
}
void Node::set(const char *value) {
	if (isValue()) {
		detach();
		data->type = T_STRING;
		data->valueStr = unescapeString(std::string(value));
	}
}
#define SET_NUMBER                   \
	if (isValue()) {                 \
		detach();                    \
		data->type = T_NUMBER;       \
		std::stringstream sstr;      \
		sstr << value;               \
		data->valueStr = sstr.str(); \
	}
void Node::set(int value) {
	SET_NUMBER
}
void Node::set(unsigned int value) {
	SET_NUMBER
}
void Node::set(long long value) {
	SET_NUMBER
}
void Node::set(unsigned long long value) {
	SET_NUMBER
}
void Node::set(float value) {
	SET_NUMBER
}
void Node::set(double value) {
	SET_NUMBER
}
#undef SET_NUMBER
void Node::set(bool value) {
	if (isValue()) {
		detach();
		data->type = T_BOOL;
		data->valueStr = (value ? "true" : "false");
	}
}

Node &Node::operator=(const Node &rhs) {
	if (this != &rhs) {
		if (data != NULL && data->release()) {
			delete data;
		}
		data = rhs.data;
		if (data != NULL) {
			data->addRef();
		}
	}
	return *this;
}
Node &Node::operator=(const std::string &rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(const char *rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(int rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(unsigned int rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(long long rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(unsigned long long rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(float rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(double rhs) {
	set(rhs);
	return *this;
}
Node &Node::operator=(bool rhs) {
	set(rhs);
	return *this;
}

void Node::add(const Node &node) {
	if (isArray()) {
		detach();
		data->children.push_back(std::make_pair(std::string(), node));
	}
}
void Node::add(const std::string &name, const Node &node) {
	if (isObject()) {
		detach();
		data->children.push_back(std::make_pair(name, node));
	}
}
void Node::append(const Node &node) {
	if ((isObject() && node.isObject()) || (isArray() && node.isArray())) {
		detach();
		data->children.insert(data->children.end(), node.data->children.begin(), node.data->children.end());
	}
}
void Node::remove(size_t index) {
	if (isContainer() && index < data->children.size()) {
		detach();
		NamedNodeList::iterator it = data->children.begin() + index;
		data->children.erase(it);
	}
}
void Node::remove(const std::string &name) {
	if (isObject()) {
		detach();
		NamedNodeList &children = data->children;
		for (NamedNodeList::iterator it = children.begin(); it != children.end(); ++it) {
			if ((*it).first == name) {
				children.erase(it);
				break;
			}
		}
	}
}
void Node::clear() {
	if (data != NULL && !data->children.empty()) {
		detach();
		data->children.clear();
	}
}

bool Node::has(const std::string &name) const {
	if (isObject()) {
		NamedNodeList &children = data->children;
		for (NamedNodeList::const_iterator it = children.begin(); it != children.end(); ++it) {
			if ((*it).first == name) {
				return true;
			}
		}
	}
	return false;
}
size_t Node::getCount() const {
	return data != NULL ? data->children.size() : 0;
}
Node Node::get(const std::string &name) const {
	if (isObject()) {
		NamedNodeList &children = data->children;
		for (NamedNodeList::const_iterator it = children.begin(); it != children.end(); ++it) {
			if ((*it).first == name) {
				return (*it).second;
			}
		}
	}
	return Node(T_INVALID);
}
Node Node::get(size_t index) const {
	if (isContainer() && index < data->children.size()) {
		return data->children.at(index).second;
	}
	return Node(T_INVALID);
}

Node::iterator Node::begin() {
	if (data != NULL && !data->children.empty())
		return Node::iterator(&data->children.front());
	else
		return Node::iterator(NULL);
}
Node::const_iterator Node::begin() const {
	if (data != NULL && !data->children.empty())
		return Node::const_iterator(&data->children.front());
	else
		return Node::const_iterator(NULL);
}
Node::iterator Node::end() {
	if (data != NULL && !data->children.empty())
		return Node::iterator(&data->children.back() + 1);
	else
		return Node::iterator(NULL);
}
Node::const_iterator Node::end() const {
	if (data != NULL && !data->children.empty())
		return Node::const_iterator(&data->children.back() + 1);
	else
		return Node::const_iterator(NULL);
}

bool Node::operator==(const Node &other) const {
	return (
			(data == other.data) ||
			(isValue() && (data->type == other.data->type) && (data->valueStr == other.data->valueStr)));
}
bool Node::operator!=(const Node &other) const {
	return !(*this == other);
}

Node::Data::Data(Type type) :
		refCount(1),
		type(type) {
}
Node::Data::Data(const Data &other) :
		refCount(1),
		type(other.type),
		valueStr(other.valueStr),
		children(other.children) {
}
Node::Data::~Data() {
	assert(refCount == 0);
}
void Node::Data::addRef() {
	++refCount;
}
bool Node::Data::release() {
	return (--refCount == 0);
}

std::string escapeString(const std::string &value) {
	std::string escaped;
	escaped.reserve(value.length());

	for (std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
		const char &c = (*it);

		const char *a = getEscaped(c);
		if (a[0] != '\0') {
			escaped += a[0];
			escaped += a[1];
		} else {
			escaped += c;
		}
	}

	return escaped;
}
std::string unescapeString(const std::string &value) {
	std::string unescaped;

	for (std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
		const char c = (*it);
		char c2 = '\0';
		if (it + 1 != value.end())
			c2 = *(it + 1);

		const char a = getUnescaped(c, c2);
		if (a != '\0') {
			unescaped += a;
			if (it + 1 != value.end())
				++it;
		} else {
			unescaped += c;
		}
	}

	return unescaped;
}

Node invalid() {
	return Node(Node::T_INVALID);
}
Node null() {
	return Node(Node::T_NULL);
}
Node object() {
	return Node(Node::T_OBJECT);
}
Node array() {
	return Node(Node::T_ARRAY);
}

Writer::Writer(const Format &format) {
	setFormat(format);
}
Writer::~Writer() {
}

void Writer::setFormat(const Format &format) {
	this->format = format;
	indentationChar = (format.useTabs ? '\t' : ' ');
	spacing = (format.spacing ? " " : "");
	newline = (format.newline ? "\n" : spacing);
}

void Writer::writeStream(const Node &node, std::ostream &stream) const {
	writeNode(node, 0, stream);
}
void Writer::writeString(const Node &node, std::string &json) const {
	std::ostringstream stream(json);
	writeStream(node, stream);
	json = stream.str();
}
void Writer::writeFile(const Node &node, const std::string &filename) const {
	std::ofstream stream(filename.c_str(), std::ios::out | std::ios::trunc);
	writeStream(node, stream);
}

void Writer::writeNode(const Node &node, unsigned int level, std::ostream &stream) const {
	switch (node.getType()) {
		case Node::T_INVALID: break;
		case Node::T_OBJECT: writeObject(node, level, stream); break;
		case Node::T_ARRAY: writeArray(node, level, stream); break;
		case Node::T_NULL: // Fallthrough
		case Node::T_STRING: // Fallthrough
		case Node::T_NUMBER: // Fallthrough
		case Node::T_BOOL: writeValue(node, stream); break;
	}
}
void Writer::writeObject(const Node &node, unsigned int level, std::ostream &stream) const {
	stream << "{" << newline;

	for (Node::const_iterator it = node.begin(); it != node.end(); ++it) {
		const std::string &name = (*it).first;
		const Node &value = (*it).second;

		if (it != node.begin())
			stream << "," << newline;
		stream << getIndentation(level + 1) << "\"" << name << "\""
			   << ":" << spacing;
		writeNode(value, level + 1, stream);
	}

	stream << newline << getIndentation(level) << "}";
}
void Writer::writeArray(const Node &node, unsigned int level, std::ostream &stream) const {
	stream << "[" << newline;

	for (Node::const_iterator it = node.begin(); it != node.end(); ++it) {
		const Node &value = (*it).second;

		if (it != node.begin())
			stream << "," << newline;
		stream << getIndentation(level + 1);
		writeNode(value, level + 1, stream);
	}

	stream << newline << getIndentation(level) << "]";
}
void Writer::writeValue(const Node &node, std::ostream &stream) const {
	if (node.isString()) {
		stream << "\"" << escapeString(node.toString()) << "\"";
	} else {
		stream << node.toString();
	}
}

std::string Writer::getIndentation(unsigned int level) const {
	if (!format.newline) {
		return "";
	} else {
		return std::string(format.indentSize * level, indentationChar);
	}
}

Parser::Parser() {
}
Parser::~Parser() {
}

Node Parser::parseStream(std::istream &stream) {
	TokenQueue tokens;
	DataQueue data;

	tokenize(stream, tokens, data);
	Node node = assemble(tokens, data);

	return node;
}
Node Parser::parseString(const std::string &json) {
	std::istringstream stream(json);
	return parseStream(stream);
}
Node Parser::parseFile(const std::string &filename) {
	std::ifstream stream(filename.c_str(), std::ios::in);
	return parseStream(stream);
}

const std::string &Parser::getError() const {
	return error;
}

void Parser::tokenize(std::istream &stream, TokenQueue &tokens, DataQueue &data) {
	Token token = T_UNKNOWN;
	std::string valueBuffer;
	bool saveBuffer;

	char c = '\0';
	while (stream.peek() != std::char_traits<char>::eof()) {
		stream.get(c);

		if (isWhitespace(c))
			continue;

		saveBuffer = true;

		switch (c) {
			case '{': {
				token = T_OBJ_BEGIN;
				break;
			}
			case '}': {
				token = T_OBJ_END;
				break;
			}
			case '[': {
				token = T_ARRAY_BEGIN;
				break;
			}
			case ']': {
				token = T_ARRAY_END;
				break;
			}
			case ',': {
				token = T_SEPARATOR_NODE;
				break;
			}
			case ':': {
				token = T_SEPARATOR_NAME;
				break;
			}
			case '"': {
				token = T_VALUE;
				readString(stream, data);
				break;
			}
			case '/': {
				char p = static_cast<char>(stream.peek());
				if (p == '*') {
					jumpToCommentEnd(stream);
					saveBuffer = false;
					break;
				} else if (p == '/') {
					jumpToNext('\n', stream);
					saveBuffer = false;
					break;
				}
				// Intentional fallthrough
			}
			default: {
				valueBuffer += c;
				saveBuffer = false;
				break;
			}
		}

		if ((saveBuffer || stream.peek() == std::char_traits<char>::eof()) && (!valueBuffer.empty())) // Always save buffer on the last character
		{
			if (interpretValue(valueBuffer, data)) {
				tokens.push(T_VALUE);
			} else {
				// Store the unknown token, so we can show it to the user
				data.push(std::make_pair(Node::T_STRING, valueBuffer));
				tokens.push(T_UNKNOWN);
			}

			valueBuffer.clear();
		}

		// Push the token last so that any data
		// will get pushed first from above.
		// If saveBuffer is false, it means that
		// we are in the middle of a value, so we
		// don't want to push any tokens now.
		if (saveBuffer) {
			tokens.push(token);
		}
	}
}
Node Parser::assemble(TokenQueue &tokens, DataQueue &data) {
	std::stack<NamedNode> nodeStack;
	Node root(Node::T_INVALID);

	std::string nextName = "";

	Token token;
	while (!tokens.empty()) {
		token = tokens.front();
		tokens.pop();

		switch (token) {
			case T_UNKNOWN: {
				const std::string &unknownToken = data.front().second;
				error = "Unknown token: " + unknownToken;
				data.pop();
				return Node(Node::T_INVALID);
			}
			case T_OBJ_BEGIN: {
				nodeStack.push(std::make_pair(nextName, object()));
				nextName.clear();
				break;
			}
			case T_ARRAY_BEGIN: {
				nodeStack.push(std::make_pair(nextName, array()));
				nextName.clear();
				break;
			}
			case T_OBJ_END:
			case T_ARRAY_END: {
				if (nodeStack.empty()) {
					error = "Found end of object or array without beginning";
					return Node(Node::T_INVALID);
				}
				if (token == T_OBJ_END && !nodeStack.top().second.isObject()) {
					error = "Mismatched end and beginning of object";
					return Node(Node::T_INVALID);
				}
				if (token == T_ARRAY_END && !nodeStack.top().second.isArray()) {
					error = "Mismatched end and beginning of array";
					return Node(Node::T_INVALID);
				}

				std::string nodeName = nodeStack.top().first;
				Node node = nodeStack.top().second;
				nodeStack.pop();

				if (!nodeStack.empty()) {
					Node &stackTop = nodeStack.top().second;
					if (stackTop.isObject()) {
						stackTop.add(nodeName, node);
					} else if (stackTop.isArray()) {
						stackTop.add(node);
					} else {
						error = "Can only add elements to objects and arrays";
						return Node(Node::T_INVALID);
					}
				} else {
					root = node;
				}
				break;
			}
			case T_VALUE: {
				if (data.empty()) {
					error = "Missing data for value";
					return Node(Node::T_INVALID);
				}

				const std::pair<Node::Type, std::string> &dataPair = data.front();
				if (!tokens.empty() && tokens.front() == T_SEPARATOR_NAME) {
					tokens.pop();
					if (dataPair.first != Node::T_STRING) {
						error = "A name has to be a string";
						return Node(Node::T_INVALID);
					} else {
						nextName = dataPair.second;
						data.pop();
					}
				} else {
					Node node(dataPair.first, dataPair.second);
					data.pop();

					if (!nodeStack.empty()) {
						Node &stackTop = nodeStack.top().second;
						if (stackTop.isObject())
							stackTop.add(nextName, node);
						else if (stackTop.isArray())
							stackTop.add(node);

						nextName.clear();
					} else {
						error = "Outermost node must be an object or array";
						return Node(Node::T_INVALID);
					}
				}
				break;
			}
			case T_SEPARATOR_NAME:
				break;
			case T_SEPARATOR_NODE: {
				if (!tokens.empty() && tokens.front() == T_ARRAY_END) {
					error = "Extra comma in array";
					return Node(Node::T_INVALID);
				}
				break;
			}
		}
	}

	return root;
}

void Parser::jumpToNext(char c, std::istream &stream) {
	while (!stream.eof() && static_cast<char>(stream.get()) != c)
		;
	stream.unget();
}
void Parser::jumpToCommentEnd(std::istream &stream) {
	stream.ignore(1);
	char c1 = '\0', c2 = '\0';
	while (stream.peek() != std::char_traits<char>::eof()) {
		stream.get(c2);

		if (c1 == '*' && c2 == '/')
			break;

		c1 = c2;
	}
}

void Parser::readString(std::istream &stream, DataQueue &data) {
	std::string str;

	char c1 = '\0', c2 = '\0';
	while (stream.peek() != std::char_traits<char>::eof()) {
		stream.get(c2);

		if (c1 != '\\' && c2 == '"') {
			break;
		}

		str += c2;

		c1 = c2;
	}

	data.push(std::make_pair(Node::T_STRING, str));
}
bool Parser::interpretValue(const std::string &value, DataQueue &data) {
	std::string upperValue(value.size(), '\0');

	std::transform(value.begin(), value.end(), upperValue.begin(), toupper);

	if (upperValue == "NULL") {
		data.push(std::make_pair(Node::T_NULL, std::string()));
	} else if (upperValue == "TRUE") {
		data.push(std::make_pair(Node::T_BOOL, std::string("true")));
	} else if (upperValue == "FALSE") {
		data.push(std::make_pair(Node::T_BOOL, std::string("false")));
	} else {
		bool number = true;
		bool negative = false;
		bool fraction = false;
		bool scientific = false;
		bool scientificSign = false;
		bool scientificNumber = false;
		for (std::string::const_iterator it = upperValue.begin(); number && it != upperValue.end(); ++it) {
			char c = (*it);
			switch (c) {
				case '-': {
					if (scientific) {
						if (scientificSign) // Only one - allowed after E
							number = false;
						else
							scientificSign = true;
					} else {
						if (negative) // Only one - allowed before E
							number = false;
						else
							negative = true;
					}
					break;
				}
				case '+': {
					if (!scientific || scientificSign)
						number = false;
					else
						scientificSign = true;
					break;
				}
				case '.': {
					if (fraction) // Only one . allowed
						number = false;
					else
						fraction = true;
					break;
				}
				case 'E': {
					if (scientific)
						number = false;
					else
						scientific = true;
					break;
				}
				default: {
					if (c >= '0' && c <= '9') {
						if (scientific)
							scientificNumber = true;
					} else {
						number = false;
					}
					break;
				}
			}
		}

		if (scientific && !scientificNumber)
			number = false;

		if (number) {
			data.push(std::make_pair(Node::T_NUMBER, value));
		} else {
			return false;
		}
	}

	return true;
}
} // namespace JSON
