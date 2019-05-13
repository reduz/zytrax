#include "json_file.h"

Error save_json(const String &p_path, const JSON::Node &p_node) {
	JSON::Writer w;
	std::string str;
	w.setFormat(JSON::StandardFormat);
	w.writeString(p_node, str);

#ifdef WINDOWS_ENABLED
	FILE *f = _wfopen(p_path.c_str(), L"wb");
#else
	FILE *f = fopen(p_path.utf8().get_data(), "wb");
#endif
	ERR_FAIL_COND_V(!f, ERR_FILE_CANT_OPEN);
	fwrite(&str[0], str.length(), 1, f);
	fclose(f);
	return OK;
}
Error load_json(const String &p_path, JSON::Node &p_node) {

#ifdef WINDOWS_ENABLED
	FILE *f = _wfopen(p_path.c_str(), L"rb");
#else
	FILE *f = fopen(p_path.utf8().get_data(), "rb");
#endif
	ERR_FAIL_COND_V(!f, ERR_FILE_CANT_OPEN);

	std::string str;

	fseek(f, 0, SEEK_END);
	size_t pos = ftell(f);
	str.resize(pos);
	fseek(f, 0, SEEK_SET);
	fread(&str[0], pos, 1, f);

	JSON::Parser p;
	p_node = p.parseString(str);

	fclose(f);

	return OK;
}
