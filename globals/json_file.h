#ifndef JSON_FILE_H
#define JSON_FILE_H

#include "error_list.h"
#include "error_macros.h"
#include "json.h"
#include "rstring.h"

Error save_json(const String &p_path, const JSON::Node &p_node);
Error load_json(const String &p_path, JSON::Node &p_node);

#endif // JSON_FILE_H
