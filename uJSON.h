/*
 * MIT License
 *
 * Copyright (c) 2022 Sam Liu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef UJSON_H__
#define UJSON_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#define UJSON_API extern


typedef enum {
  UJSON_UNDEFINED = 0,
  UJSON_OBJECT = 1,
  UJSON_ARRAY = 2,
  UJSON_STRING = 3,
  UJSON_PRIMITIVE = 4
} uJsonNodeType;

typedef enum  {
    UJSON_OK = 0,
  /* Not enough tokens were provided */
  UJSON_ERROR_NOMEM = -1,
  /* Invalid character inside JSON string */
  UJSON_ERROR_INVAL = -2,
  /* The string is not a full JSON packet, more bytes expected */
  UJSON_ERROR_PART = -3,
  UJSON_ERROR_KEY = -4,
  UJSON_ERROR_VALUE = -5,
  UJSON_ERROR_SPACE = -6,
  UJSON_ERROR_TYPE = -7,
}uJsonError;


UJSON_API uJsonError uJson_parse(const char *js);

UJSON_API void uJson_dumps(void);

UJSON_API uJsonError uJson_get_string(char* s, int size, ...);
UJSON_API uJsonError uJson_get_string_size(int* size, ...);

UJSON_API uJsonError uJson_get_integer(int *value, ...);
UJSON_API uJsonError uJson_get_real(double *value, ...);

UJSON_API uJsonError uJson_get_items(int* items, ...);

UJSON_API uJsonError uJson_create_root_object(char* buf, int size);
UJSON_API uJsonError uJson_create_root_array(char* buf, int size);

UJSON_API uJsonError uJson_add_item(uJsonNodeType type, char *key, char *val,  ...);

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H__ */
