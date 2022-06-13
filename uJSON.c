#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "uJSON.h"

#define TOKEN_NUMS 256

// #define UJSON_DEBUG DEBUG
#define UJSON_DEBUG


/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct {
	uJsonNodeType type;
	int key_start;
	int key_end;
	int val_start;
	int val_end;
	int level;
} uJsonNode;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct {
	unsigned int pos;     /* offset in the JSON string */
	int toknext; /* next token to allocate */
	char* text;
    int space_of_text;
} uJsonParser;

static uJsonParser parser;
uJsonNode tokens[TOKEN_NUMS]; /* We expect no more than 128 tokens */


uJsonError uJson_parse_object(uJsonNode* owner, int level);

/**
 * Create JSON parser over an array of tokens
 */
static void uJson_init(const char* s)
{
	parser.pos = 0;
	parser.toknext = 0;
	parser.text = (char *)s;
}

/**
 * Allocates a fresh unused token from the token pool.
 */
static uJsonNode* uJson_alloc_token(uJsonNodeType type, int key_start, int key_end, int level) {
	char s1[128] = { "\0" };

	uJsonNode* tok;
	if (parser.toknext >= TOKEN_NUMS) {
		return NULL;
	}
	tok = &tokens[parser.toknext++];
	tok->type = type;
	tok->key_start = key_start;
	tok->key_end = key_end;
	tok->val_start = -1;
	tok->val_end = -1;

	tok->level = level;

	strncpy(s1, (parser.text + key_start), key_end - key_start);
	UJSON_DEBUG("%s -> ...", s1);

	return tok;
}

static int uJson_save_token(uJsonNodeType type, int key_start, int key_end, int val_start, int val_end, int level) {
	char s1[128] = { "\0" };
	char s2[128] = { "\0" };
	int n;

	uJsonNode* tok;
	if (parser.toknext >= TOKEN_NUMS) {
		return 0;
	}
	tok = &tokens[parser.toknext++];
	tok->type = type;
	tok->key_start = key_start;
	tok->key_end = key_end;
	tok->val_start = val_start;
	tok->val_end = val_end;

	tok->level = level;

	n = key_end - key_start;
	strncpy(s1, (parser.text + key_start), n > 127 ? 127 : n);
	n = val_end - val_start;
	strncpy(s2, parser.text + val_start, n > 127 ? 127 : n);
	UJSON_DEBUG("%s -> %s", s1, s2);
	return 1;
}


int uJson_parse_quoted_string()
{
	for (parser.pos++; parser.text[parser.pos]; parser.pos++) {
		if (parser.text[parser.pos] == '"') {
			if (parser.text[parser.pos - 1] != '\\') {
				return 1;
			}
		}
	}

	return 0;
}

int uJson_parse_unbroken_text()
{
	for (parser.pos++; parser.text[parser.pos]; parser.pos++) {
		if (strchr(",}] \t\r\n", parser.text[parser.pos]) != NULL) {
			return 1;
		}
	}

	return 0;
}

uJsonError uJson_parse_array(uJsonNode* owner, int level)
{
	uJsonError r = UJSON_ERROR_PART;
	int val_start = 0;
	int val_end = 0;
	uJsonNode* token;

	level++;
	owner->val_start = parser.pos;

	for (parser.pos++; parser.text[parser.pos] && r == UJSON_ERROR_PART; parser.pos++) {
		switch (parser.text[parser.pos]) {
		case '"':
			if (val_start && val_end) {
				r = UJSON_ERROR_INVAL;
			}
			else {
				val_start = parser.pos;
				if (uJson_parse_quoted_string() == 1) {
					val_end = parser.pos;
					if (0 == uJson_save_token(UJSON_STRING, val_start + 1, val_end, val_start + 1, val_end, level)) {
						r = UJSON_ERROR_NOMEM;
					}
				}
			}
			break;
		case '{':
			token = uJson_alloc_token(UJSON_OBJECT, parser.pos, parser.pos, level);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				switch (uJson_parse_object(token, level)) {
				case UJSON_ERROR_INVAL:
					r = UJSON_ERROR_INVAL;
					break;
				case UJSON_ERROR_NOMEM:
					r = UJSON_ERROR_NOMEM;
					break;
				default:
					break;
				}
			}
			break;
		case '[':
			token = uJson_alloc_token(UJSON_ARRAY, parser.pos, parser.pos, level);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				switch (uJson_parse_array(token, level)) {
				case UJSON_ERROR_INVAL:
					r = UJSON_ERROR_INVAL;
					break;
				case UJSON_ERROR_NOMEM:
					r = UJSON_ERROR_NOMEM;
					break;
				default:
					break;
				}
			}
			break;
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;
		case ',':
			//next item
			val_start = 0;
			val_end = 0;
			break;
		case ']':
			r = UJSON_OK;
			owner->val_end = parser.pos;
			parser.pos--;
			break;
		default:
			if (val_start && val_end) {
				r = UJSON_ERROR_INVAL;
			}
			else {
				val_start = parser.pos;
				if (uJson_parse_unbroken_text()) {
					val_end = parser.pos;
					// save a string token here
					if (0 == uJson_save_token(UJSON_PRIMITIVE, val_start, val_end, val_start, val_end, level)) {
						r = UJSON_ERROR_NOMEM;
					}
					parser.pos--;
				}
			}
			break;
		}
	}

	owner->val_end = parser.pos;

	return UJSON_OK;
}



uJsonError uJson_parse_object(uJsonNode* owner, int level)
{
	uJsonError r = UJSON_ERROR_PART;
	int key_start = 0;
	int key_end = 0;
	int val_start = 0;
	int val_end = 0;
	int sep = 0;
	uJsonNode* token;

	level++;
	owner->val_start = parser.pos;

	for (parser.pos++; parser.text[parser.pos] && r == UJSON_ERROR_PART; parser.pos++) {
		switch (parser.text[parser.pos]) {
		case '"':
			if (sep && key_start && key_end && val_start && val_end) {
				r = UJSON_ERROR_INVAL;;
			}
			else if (key_start == 0 && key_end == 0) {
				key_start = parser.pos;
				if (uJson_parse_quoted_string() == 1) {
					key_end = parser.pos;
				}
			}
			else if (sep && key_start && key_end) {
				// with char ':', string value
				val_start = parser.pos;
				if (uJson_parse_quoted_string() == 1) {
					val_end = parser.pos;
					// save a string token here
					if (0 == uJson_save_token(UJSON_STRING, key_start + 1, key_end, val_start + 1, val_end, level)) {
						r = UJSON_ERROR_NOMEM;
					}
				}
			}
			break;
		case '{':
			token = uJson_alloc_token(UJSON_OBJECT, key_start + 1, key_end, level);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				switch (uJson_parse_object(token, level)) {
				case UJSON_ERROR_INVAL:
					r = UJSON_ERROR_INVAL;
					break;
				case UJSON_ERROR_NOMEM:
					r = UJSON_ERROR_NOMEM;
					break;
				default:
					break;
				}
			}
			break;
		case '[':
			token = uJson_alloc_token(UJSON_ARRAY, key_start + 1, key_end, level);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				switch (uJson_parse_array(token, level)) {
				case UJSON_ERROR_INVAL:
					r = UJSON_ERROR_INVAL;
					break;
				case UJSON_ERROR_NOMEM:
					r = UJSON_ERROR_NOMEM;
					break;
				default:
					break;
				}
			}
			break;
		case ':':
			sep = parser.pos;
			break;
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;
		case ',':
			//next item
			key_start = 0;
			key_end = 0;
			sep = 0;
			val_start = 0;
			val_end = 0;
			break;
		case '}':
			owner->val_end = parser.pos;
			r = UJSON_OK;
			parser.pos--;
			break;
		default:
			if (sep) {
				val_start = parser.pos;
				if (uJson_parse_unbroken_text()) {
					val_end = parser.pos;
					// save a string token here
					if (0 == uJson_save_token(UJSON_PRIMITIVE, key_start + 1, key_end, val_start, val_end, level)) {
						r = UJSON_ERROR_NOMEM;
					}
					parser.pos--;
				}
			}
			else {
				r = UJSON_ERROR_INVAL;
			}
			break;
		}
	}

	return UJSON_OK;
}





void uJson_dumps()
{
	int i, j;
	char prefix[128] = { 0 };
	char buf[5120] = { 0 };
	char* p;

	for (i = 0; i < TOKEN_NUMS; i++) {
		if (i < parser.toknext) {
			memset(prefix, 0, 128);
			memset(buf, 0, 256);
			p = prefix;
			for (j = 0; j < tokens[i].level; j++) {
				strcpy(p, "    ");
				p += 4;
			}
			switch (tokens[i].type) {
			case UJSON_OBJECT:
				UJSON_DEBUG("%sType: Object", prefix);
				break;
			case UJSON_ARRAY:
				UJSON_DEBUG("%sType: Array", prefix);
				break;
			case UJSON_STRING:
				UJSON_DEBUG("%sType: String", prefix);
				break;
			case UJSON_PRIMITIVE:
				UJSON_DEBUG("%sType: Primitive", prefix);
				break;
			}
			if (tokens[i].key_end < tokens[i].key_start || tokens[i].val_end < tokens[i].val_start) {
				UJSON_DEBUG("test");
			}
			strncpy(buf, (parser.text + tokens[i].key_start), tokens[i].key_end - tokens[i].key_start);
			UJSON_DEBUG("%sKey:%s", prefix, buf);
			memset(buf, 0, 256);
			strncpy(buf, (parser.text + tokens[i].val_start), tokens[i].val_end - tokens[i].val_start);
			UJSON_DEBUG("%sVal:%s", prefix, buf);
		}
	}
}


/**
 * Parse JSON string and fill tokens.
 */
UJSON_API uJsonError uJson_parse(const char* js) {
	uJsonError r = UJSON_ERROR_PART;
	uJsonNode* token = NULL;

	uJson_init(js);

	for (; parser.text[parser.pos] && r == UJSON_ERROR_PART && token == NULL; parser.pos++) {
		switch (parser.text[parser.pos]) {
		case '{':
			token = uJson_alloc_token(UJSON_OBJECT, parser.pos, parser.pos, 0);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				r = uJson_parse_object(token, 0);
			}
			break;
		case '[':
			token = uJson_alloc_token(UJSON_ARRAY, parser.pos, parser.pos, 0);
			if (token == NULL) {
				r = UJSON_ERROR_NOMEM;
			}
			else {
				r = uJson_parse_array(token, 0);
			}
			break;
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;
		default:
			r = UJSON_ERROR_INVAL;
			break;
		}
	}

	return r;
}



static uJsonError uJson_find(int* location, va_list args)
{
	int i, len;
	int level = 0;
	char* key = 0;
	char* index = 0;
	char is_obj_member = 0;
	char* p;
	uJsonError err = UJSON_ERROR_KEY;

	*location = -1;

	for (i = 0; i < parser.toknext && err == UJSON_ERROR_KEY; i++, index++) {
		if (tokens[i].level < level) {
			break;
		}
		if (tokens[i].level > level) {
			continue;
		}
		p = parser.text + tokens[i].key_start;
		len = tokens[i].key_end - tokens[i].key_start;

		if ((is_obj_member && len == strlen(key) && strncmp(key, p, len) == 0)
			|| (!is_obj_member && key == index)) {
			// match, prepare the next level
			key = va_arg(args, char*);
			if (key == (char*)-1) {
				// the end
				*location = i;
				err = UJSON_OK;
			}
			else {
				level++;
				switch (tokens[i].type) {
				case UJSON_OBJECT:
					// go to next level
					is_obj_member = 1;
					break;
				case UJSON_ARRAY:
					// go to next level
					is_obj_member = 0;
					index = 0;
					key++;
					break;
				case UJSON_STRING:
				case UJSON_PRIMITIVE:
					break;
				}
			}
		}
	}

	return err;
}


uJsonError uJson_get_string(char* s, int size, ...)
{
	int index, len;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, size);

	if (UJSON_OK == uJson_find(&index, argptr)) {
		memset(s, 0, size);
		len = tokens[index].val_end - tokens[index].val_start;
		if (len > size - 1) {
			len = size - 1;
			err = UJSON_ERROR_SPACE;
		}
		else {
			err = UJSON_OK;
		}
		strncpy(s, (parser.text + tokens[index].val_start), len);
	}
	else {
		err = UJSON_ERROR_KEY;
	}

	va_end(argptr);

	return err;
}

uJsonError uJson_get_string_size(int* size, ...)
{
	int index;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, size);

	if (UJSON_OK == uJson_find(&index, argptr)) {
		*size = tokens[index].val_end - tokens[index].val_start;
		err = UJSON_OK;
	}
	else {
		err = UJSON_ERROR_KEY;
	}

	va_end(argptr);

	return err;
}

uJsonError uJson_get_integer(int* value, ...)
{
	int index;
	char* ep = NULL, * sp;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, value);

	if (UJSON_OK == uJson_find(&index, argptr)) {
		sp = parser.text + tokens[index].val_start;
		*value = (int)strtol(sp, &ep, 0);
		if (ep == parser.text + tokens[index].val_end) {
			err = UJSON_OK;
		}
		else {
			err = UJSON_ERROR_VALUE;
		}
	}
	else {
		err = UJSON_ERROR_KEY;
	}

	va_end(argptr);

	return err;
}

uJsonError uJson_get_real(double* value, ...)
{
	int index;
	char* ep = NULL, * sp;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, value);

	if (UJSON_OK == uJson_find(&index, argptr)) {
		sp = parser.text + tokens[index].val_start;
		*value = (int)strtod(sp, &ep);
		if (ep == parser.text + tokens[index].val_end) {
			err = UJSON_OK;
		}
		else {
			err = UJSON_ERROR_VALUE;
		}
	}
	else {
		err = UJSON_ERROR_KEY;
	}

	va_end(argptr);

	return err;
}


uJsonError uJson_get_items(int* items, ...)
{
	int index = 0, level = 0;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, items);

	err = uJson_find(&index, argptr);
	if (err == UJSON_OK) {
		switch (tokens[index].type) {
		case UJSON_OBJECT:
		case UJSON_ARRAY:
			*items = 0;
			level = tokens[index].level + 1;
			index += 1;
			for (; index < parser.toknext && tokens[index].level == level; index++, *items += 1);
			err = UJSON_OK;
			break;
		case UJSON_STRING:
		case UJSON_PRIMITIVE:
			err = UJSON_ERROR_TYPE;
			break;
		}
	}

	va_end(argptr);

	return err;
}

uJsonError uJson_create_root_object(char* buf, int size)
{
	memset(buf, 0, size);
	strcpy(buf, "{}");
    parser.space_of_text = size - 1;
	return uJson_parse(buf);
}

uJsonError uJson_create_root_array(char* buf, int size)
{
	memset(buf, 0, size);
	strcpy(buf, "[]");
    parser.space_of_text = size - 1;
	return uJson_parse(buf);
}

static int uJson_quote_string(char* in, char* out)
{
	int i;
    if(out != NULL){
        *out = '"';
        out++;
    }
	for (i = 0; *in; in++, i++) {
		switch (*in) {
		case '\r':
		case '\n':
		case '"':
		case '\t':
		case '\\':
			i++;
			if (out != NULL) {
				*out = '\\';
				out++;
			}
			break;
		default:
			break;
		}

		if (out != NULL) {
			*out = *in;
			out++;
		}
	}
    if(out != NULL){
        *out = '"';
        out++;
    }

	return i+2;
}

uJsonError uJson_add_item(uJsonNodeType type, char* key, char* val, ...)
{
	int new_text_bytes;
	int index;
	int dest, src;
	int len_key, len_val, colon;
	char not_empty = 1;
	va_list argptr;
	uJsonError err = UJSON_ERROR_KEY;

	va_start(argptr, val);
	err = uJson_find(&index, argptr);
	if (err == UJSON_OK) {
		if (tokens[index].val_start + 1 == tokens[index].val_end) {
			not_empty = 0;
		}

        if( tokens[index].type == UJSON_ARRAY ){
            len_key = 0;
			colon = 0;
        }
        else{
            len_key = uJson_quote_string(key, NULL);
			colon = 1;
        }
		switch (type) {
		case UJSON_OBJECT:
            val = "{}";
            len_val = 2;
			break;
		case UJSON_ARRAY:
            val = "[]";
            len_val = 2;
			break;
		case UJSON_STRING:
            len_val = uJson_quote_string(val, NULL);
			break;
		case UJSON_PRIMITIVE:
			len_val = strlen(val);
			break;
		}

        
        new_text_bytes = not_empty + len_key + colon + len_val;
        src = tokens[index].val_end;
        dest = src + new_text_bytes;
        memmove(parser.text + dest, parser.text + src, strlen(parser.text) - tokens[index].val_end);
        if (not_empty) {
            parser.text[src] = ',';
            src++;
        }
        // copy key
        if( len_key ){
            uJson_quote_string(key, parser.text + src);
            
            src += len_key;
            // set ':'
            parser.text[src] = ':';
            src++;
        }
        // copy val
        if( type == UJSON_STRING ){
            uJson_quote_string(val, parser.text + src);
        }
        else{
            memcpy(parser.text + src,val, len_val);
        }
        
        err = uJson_parse(parser.text);
	}
	va_end(argptr);

	return err;
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
