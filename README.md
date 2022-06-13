# uJSON
====

Why uJSON?
----------

Recently, I needed a json parser for my project. The project is for an embeded system and its resorce is limited. 
I studied server open json parser. But none of them met my requirement. Some need malloc/free functions, some are hard to use. 

Finally, I decided to write the json parser by my self. 

Thanks [zserge](https://github.com/zserge), who gave me the idea to implement the parser quickly. 

Philosophy
----------

Inspired by [jsmn](https://github.com/zserge/jsmn), the philosophy of uJson likes the philosophy of jsmn. 

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
```






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
```


**Note:** Unlike JSON data types, primitive tokens are not divided into
numbers, booleans and null, because one can easily tell the type using the
first character:

* <code>'t', 'f'</code> - boolean 
* <code>'n'</code> - null
* <code>'-', '0'..'9'</code> - number


To parse a json string, it is quite easy. You can call the parser like this:
  
  const char *s = "[1, {\"key\":\"val\"}]"
  uJson_parse(s);

After that you can get the value of each node by calling uJson_get_xxx functions. These functions are:

```
  UJSON_API uJsonError uJson_get_string(char* s, int size, ...);
  UJSON_API uJsonError uJson_get_string_size(int* size, ...);

  UJSON_API uJsonError uJson_get_integer(int *value, ...);
  UJSON_API uJsonError uJson_get_real(double *value, ...);

  UJSON_API uJsonError uJson_get_items(int* items, ...);
```
The first parameter of these functions are the out parameter. In anothere word, it saves the result of getting. 

To get the value of node, we must point out the location of the node. In these functions, the location is passed by variable parameters and the end flag is -1. 

For example, if we want to get the value of "key" for "[1, {\"key\":\"val\"}]", we can call uJson_get_string like:

  uJson_get_string(buf, 10, 1, "key", -1); // 1, "key", -1 indicates the node is indexed by "key" under the object which is indexed by 1 under the root array. 


To create a json string, it quite simple, too. First, you must call uJson_create_root_xxx to create the root element. Then, you call uJson_add_item to fill the element step by step. Here is the sample to build "[1, {\"key\":\"val\"}]":

```
  char buf[128];
  uJson_create_root_array(buf, 128); // the content of buf is "[]" after this step
  uJson_add_item(UJSON_PRIMITIVE, NULL, "1", 0, -1); //  the buf changes to "[1]"
  uJson_add_item(UJSON_PRIMITIVE, NULL, "{}", 1, -1); // the buf changes to "[1, {}]"
  uJson_add_item(UJSON_STRING, "key", "val", 1, -1); // the buf changes to "[1, {\"key\":\"val\"}]" finally
```




If something goes wrong, you will get an error. Error will be one of these:

* `UJSON_ERROR_INVAL` - bad token, JSON string is corrupted
* `UJSON_ERROR_NOMEM` - not enough tokens, JSON string is too large
* `UJSON_ERROR_PART` - JSON string is too short, expecting more JSON data
* `UJSON_ERROR_KEY` - JSON key error, the key does not exist
* `UJSON_ERROR_VALUE` - JSON value error, the value is incorrect

If you get `UJSON_ERROR_NOMEM`, you can re-allocate more tokens by changing the definition of macro TOKEN_NUMS in *uJSON.c*.  If you read json data from the stream, you can
periodically call `uJson_parse` and check if return value is `UJSON_ERROR_PART`.
You will get this error until you reach the end of JSON data.

Other info
----------
This software is distributed under [MIT license](http://www.opensource.org/licenses/mit-license.php),
 so feel free to integrate it in your commercial products.
 


