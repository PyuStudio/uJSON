# uJSON
====

Why uJSON?
----------

Recently, I needed a json parser for my project. The project is for an embeded system and its resorce is limited. 
I studied server open json parser. But none of them met my requirement. Some need malloc/free functions, some are hard to use. 

Finally, I decided to write the json parser by my self. 

Thanks zserge, who gave me the idea to implement the parser quickly. 

Philosophy
----------

Inspired by jsmn[https://github.com/zserge/jsmn], the philosophy of uJson likes the philosophy of jsmn. 

uJson is also designed to be	**robust** (it should work fine even with erroneous
data), **fast** (it should parse data on the fly), **portable** (no superfluous
dependencies or non-standard C extensions). And of course, **simplicity** is a
key feature - simple code style, simple algorithm, simple integration into
other projects.

Features
--------

* compatible with C89
* no dependencies (even libc!)
* highly portable (tested on x86/amd64, ARM, AVR)
* only two files
* extremely small code footprint
* APIs of easy to use
* no dynamic memory allocation
* incremental single-pass parsing
* library code is covered with unit-tests

Design
------


The rudimentary jsmn object is a **token**. Let's consider a JSON string:

	'{ "name" : "Jack", "age" : 27 }'

It holds the following tokens:

* Object: `{ "name" : "Jack", "age" : 27}` (the whole object)
* Strings: `"name"`, `"Jack"`, `"age"` (keys and some values)
* Number: `27`

In jsmn, tokens do not hold any data, but point to token boundaries in JSON
string instead. In the example above jsmn will create tokens like: Object
[0..31], String [3..7], String [12..16], String [20..23], Number [27..29].

Every jsmn token has a type, which indicates the type of corresponding JSON
token. jsmn supports the following token types:

* Object - a container of key-value pairs, e.g.:
	`{ "foo":"bar", "x":0.3 }`
* Array - a sequence of values, e.g.:
	`[ 1, 2, 3 ]`
* String - a quoted sequence of chars, e.g.: `"foo"`
* Primitive - a number, a boolean (`true`, `false`) or `null`

Besides start/end positions, jsmn tokens for complex types (like arrays
or objects) also contain a number of child items, so you can easily follow
object hierarchy.

This approach provides enough information for parsing any JSON data and makes
it possible to use zero-copy techniques.

Usage
-----

Download `uJSON.h` and 'uJSON.c', add files to you proect, done.

```
#include "uJSON.h"

...
#define JSON_BUFFER_BYTES   5120    //1024*5
static char json_buffer[JSON_BUFFER_BYTES];

/* to build a json object */
static char* create_command(char* host, char* interface, const char* method)
{
	char url[128];

	sprintf(url, "http://%s/%s", host, interface);

	uJson_create_root_object(json_buffer, JSON_BUFFER_BYTES);

	uJson_add_item(UJSON_STRING, "url", url, -1);
	uJson_add_item(UJSON_STRING, "method", (char*)method, -1);


	return json_buffer;
}

/* to parse a json string */
uJsonError on_json(const char *s)
{
    char buf[64];
    int i, numbers, val;
    uJsonError err;
    
	  err = uJson_parse(s);
    if( err != UJSON_OK ){
        return err
    }
    
    if( uJson_get_string(buf, 64, "data", "ip", -1) == UJSON_OK ){        
		    ...
    }
    
    if( uJson_get_items(&numbers, "data", "nodeInfos", -1) == UJSON_OK ){        
        for (i = 0; i < numbers; i++) {
            if( uJson_get_integer(&val, "data", "nodeInfos", i, "id", -1) == UJSON_OK ){ 
                ...
            }
        }
    }
}






API
---

Token types are described by `uJsonNodeType`:


```

  typedef enum {
    UJSON_UNDEFINED = 0,
    UJSON_OBJECT = 1,
    UJSON_ARRAY = 2,
    UJSON_STRING = 3,
    UJSON_PRIMITIVE = 4
  } uJsonNodeType;



**Note:** Unlike JSON data types, primitive tokens are not divided into
numbers, booleans and null, because one can easily tell the type using the
first character:

* <code>'t', 'f'</code> - boolean 
* <code>'n'</code> - null
* <code>'-', '0'..'9'</code> - number

Token is an object of `jsmntok_t` type:

	typedef struct {
		jsmntype_t type; // Token type
		int start;       // Token start position
		int end;         // Token end position
		int size;        // Number of child (nested) tokens
	} jsmntok_t;

**Note:** string tokens point to the first character after
the opening quote and the previous symbol before final quote. This was made 
to simplify string extraction from JSON data.

All job is done by `jsmn_parser` object. You can initialize a new parser using:

	jsmn_parser parser;
	jsmntok_t tokens[10];

	jsmn_init(&parser);

	// js - pointer to JSON string
	// tokens - an array of tokens available
	// 10 - number of tokens available
	jsmn_parse(&parser, js, strlen(js), tokens, 10);

This will create a parser, and then it tries to parse up to 10 JSON tokens from
the `js` string.

A non-negative return value of `jsmn_parse` is the number of tokens actually
used by the parser.
Passing NULL instead of the tokens array would not store parsing results, but
instead the function will return the number of tokens needed to parse the given
string. This can be useful if you don't know yet how many tokens to allocate.

If something goes wrong, you will get an error. Error will be one of these:

* `JSMN_ERROR_INVAL` - bad token, JSON string is corrupted
* `JSMN_ERROR_NOMEM` - not enough tokens, JSON string is too large
* `JSMN_ERROR_PART` - JSON string is too short, expecting more JSON data

If you get `JSMN_ERROR_NOMEM`, you can re-allocate more tokens and call
`jsmn_parse` once more.  If you read json data from the stream, you can
periodically call `jsmn_parse` and check if return value is `JSMN_ERROR_PART`.
You will get this error until you reach the end of JSON data.

Other info
----------

This software is distributed under [MIT license](http://www.opensource.org/licenses/mit-license.php),
 so feel free to integrate it in your commercial products.

