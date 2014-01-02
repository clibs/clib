/*
 Parson ( http://kgabis.github.com/parson/ )
 Copyright (c) 2013 Krzysztof Gabis
 
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

#ifndef parson_parson_h
#define parson_parson_h

#ifdef __cplusplus
extern "C"
{
#endif    
    
#include <stddef.h>   /* size_t */    

#define PARSON_VERSION 20131130
    
/* Types and enums */
typedef struct json_object_t JSON_Object;
typedef struct json_array_t  JSON_Array;
typedef struct json_value_t  JSON_Value;

typedef enum json_value_type {
    JSONError   = 0,
    JSONNull    = 1,
    JSONString  = 2,
    JSONNumber  = 3,
    JSONObject  = 4,
    JSONArray   = 5,
    JSONBoolean = 6
} JSON_Value_Type;

   
/* Parses first JSON value in a file, returns NULL in case of error */
JSON_Value  * json_parse_file(const char *filename);

/* Parses first JSON value in a file and ignores comments (/ * * / and //),
   returns NULL in case of error */
JSON_Value  * json_parse_file_with_comments(const char *filename);
    
/*  Parses first JSON value in a string, returns NULL in case of error */
JSON_Value  * json_parse_string(const char *string);

/*  Parses first JSON value in a string and ignores comments (/ * * / and //),
    returns NULL in case of error */
JSON_Value  * json_parse_string_with_comments(const char *string);
    
/* JSON Object */
JSON_Value  * json_object_get_value  (const JSON_Object *object, const char *name);
const char  * json_object_get_string (const JSON_Object *object, const char *name);
JSON_Object * json_object_get_object (const JSON_Object *object, const char *name);
JSON_Array  * json_object_get_array  (const JSON_Object *object, const char *name);
double        json_object_get_number (const JSON_Object *object, const char *name);
int           json_object_get_boolean(const JSON_Object *object, const char *name);

/* dotget functions enable addressing values with dot notation in nested objects,
 just like in structs or c++/java/c# objects (e.g. objectA.objectB.value).
 Because valid names in JSON can contain dots, some values may be inaccessible
 this way. */
JSON_Value  * json_object_dotget_value  (const JSON_Object *object, const char *name);
const char  * json_object_dotget_string (const JSON_Object *object, const char *name);
JSON_Object * json_object_dotget_object (const JSON_Object *object, const char *name);
JSON_Array  * json_object_dotget_array  (const JSON_Object *object, const char *name);
double        json_object_dotget_number (const JSON_Object *object, const char *name);
int           json_object_dotget_boolean(const JSON_Object *object, const char *name);

/* Functions to get available names */
size_t        json_object_get_count(const JSON_Object *object);
const char  * json_object_get_name (const JSON_Object *object, size_t index);
    
/* JSON Array */
JSON_Value  * json_array_get_value  (const JSON_Array *array, size_t index);
const char  * json_array_get_string (const JSON_Array *array, size_t index);
JSON_Object * json_array_get_object (const JSON_Array *array, size_t index);
JSON_Array  * json_array_get_array  (const JSON_Array *array, size_t index);
double        json_array_get_number (const JSON_Array *array, size_t index);
int           json_array_get_boolean(const JSON_Array *array, size_t index);
size_t        json_array_get_count  (const JSON_Array *array);

/* JSON Value */
JSON_Value_Type json_value_get_type   (const JSON_Value *value);
JSON_Object *   json_value_get_object (const JSON_Value *value);
JSON_Array  *   json_value_get_array  (const JSON_Value *value);
const char  *   json_value_get_string (const JSON_Value *value);
double          json_value_get_number (const JSON_Value *value);
int             json_value_get_boolean(const JSON_Value *value);
void            json_value_free       (JSON_Value *value);
    
#ifdef __cplusplus
}
#endif

#endif
