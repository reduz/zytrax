#ifndef _BASE64_H_
#define _BASE64_H_

#include "vector.h"
#include <string>
#include <cstdint>

std::string base64_encode(const Vector<uint8_t> &p_buffer);
Vector<uint8_t> base64_decode(std::string const &);

#endif // BASE64_H
