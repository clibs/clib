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

#include "parson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ERROR                      0
#define SUCCESS                    1
#define STARTING_CAPACITY         15
#define ARRAY_MAX_CAPACITY    122880 /* 15*(2^13) */
#define OBJECT_MAX_CAPACITY      960 /* 15*(2^6)  */
#define MAX_NESTING               19
#define sizeof_token(a)       (sizeof(a) - 1)
#define skip_char(str)        ((*str)++)
#define skip_whitespaces(str) while (isspace(**str)) { skip_char(str); }
#define MAX(a, b)             ((a) > (b) ? (a) : (b))

#define parson_malloc(a)     malloc(a)
#define parson_free(a)       free((void*)a)
#define parson_realloc(a, b) realloc(a, b)

/* Type definitions */
typedef union json_value_value {
    const char  *string;
    double       number;
    JSON_Object *object;
    JSON_Array  *array;
    int          boolean;
    int          null;
} JSON_Value_Value;

struct json_value_t {
    JSON_Value_Type     type;
    JSON_Value_Value    value;
};

struct json_object_t {
    const char **names;
    JSON_Value **values;
    size_t       count;
    size_t       capacity;
};

struct json_array_t {
    JSON_Value **items;
    size_t       count;
    size_t       capacity;
};

/* Various */
static char * read_file(const char *filename);
static void   remove_comments(char *string, const char *start_token, const char *end_token);
static int    try_realloc(void **ptr, size_t new_size);
static char * parson_strndup(const char *string, size_t n);
static int    is_utf(const unsigned char *string);
static int    is_decimal(const char *string, size_t length);

/* JSON Object */
static JSON_Object * json_object_init(void);
static int           json_object_add(JSON_Object *object, const char *name, JSON_Value *value);
static int           json_object_resize(JSON_Object *object, size_t capacity);
static JSON_Value  * json_object_nget_value(const JSON_Object *object, const char *name, size_t n);
static void          json_object_free(JSON_Object *object);

/* JSON Array */
static JSON_Array * json_array_init(void);
static int          json_array_add(JSON_Array *array, JSON_Value *value);
static int          json_array_resize(JSON_Array *array, size_t capacity);
static void         json_array_free(JSON_Array *array);

/* JSON Value */
static JSON_Value * json_value_init_object(void);
static JSON_Value * json_value_init_array(void);
static JSON_Value * json_value_init_string(const char *string);
static JSON_Value * json_value_init_number(double number);
static JSON_Value * json_value_init_boolean(int boolean);
static JSON_Value * json_value_init_null(void);

/* Parser */
static void         skip_quotes(const char **string);
static const char * get_processed_string(const char **string);
static JSON_Value * parse_object_value(const char **string, size_t nesting);
static JSON_Value * parse_array_value(const char **string, size_t nesting);
static JSON_Value * parse_string_value(const char **string);
static JSON_Value * parse_boolean_value(const char **string);
static JSON_Value * parse_number_value(const char **string);
static JSON_Value * parse_null_value(const char **string);
static JSON_Value * parse_value(const char **string, size_t nesting);

/* Various */
static int try_realloc(void **ptr, size_t new_size) {
    void *reallocated_ptr = parson_realloc(*ptr, new_size);
    if (!reallocated_ptr)
        return ERROR;
    *ptr = reallocated_ptr;
    return SUCCESS;
}

static char * parson_strndup(const char *string, size_t n) {
    char *output_string = (char*)parson_malloc(n + 1);
    if (!output_string)
        return NULL;
    output_string[n] = '\0';
    strncpy(output_string, string, n);
    return output_string;
}

static int is_utf(const unsigned char *s) {
    return isxdigit(s[0]) && isxdigit(s[1]) && isxdigit(s[2]) && isxdigit(s[3]);
}

static int is_decimal(const char *string, size_t length) {
    if (length > 1 && string[0] == '0' && string[1] != '.')
        return 0;
    if (length > 2 && !strncmp(string, "-0", 2) && string[2] != '.')
        return 0;
    while (length--)
        if (strchr("xX", string[length]))
            return 0;
    return 1;
}

static char * read_file(const char * filename) {
    FILE *fp = fopen(filename, "r");
    size_t file_size;
    char *file_contents;
    if (!fp)
        return NULL;
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    file_contents = (char*)parson_malloc(sizeof(char) * (file_size + 1));
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1) {
        if (ferror(fp)) {
            fclose(fp);
            parson_free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    file_contents[file_size] = '\0';
    return file_contents;
}

static void remove_comments(char *string, const char *start_token, const char *end_token) {
    int in_string = 0, escaped = 0;
    size_t i;
    char *ptr = NULL, current_char;
    size_t start_token_len = strlen(start_token);
    size_t end_token_len = strlen(end_token);
    if (start_token_len == 0 || end_token_len == 0)
    	return;
    while ((current_char = *string) != '\0') {
        if (current_char == '\\' && !escaped) {
            escaped = 1;
            string++;
            continue;
        } else if (current_char == '\"' && !escaped) {
            in_string = !in_string;
        } else if (!in_string && strncmp(string, start_token, start_token_len) == 0) {
			for(i = 0; i < start_token_len; i++)
                string[i] = ' ';
        	string = string + start_token_len;
            ptr = strstr(string, end_token);
            if (!ptr)
                return;
            for (i = 0; i < (ptr - string) + end_token_len; i++)
                string[i] = ' ';
          	string = ptr + end_token_len - 1;
        }
        escaped = 0;
        string++;
    }
}

/* JSON Object */
static JSON_Object * json_object_init(void) {
    JSON_Object *new_obj = (JSON_Object*)parson_malloc(sizeof(JSON_Object));
    if (!new_obj)
        return NULL;
    new_obj->names = (const char**)NULL;
    new_obj->values = (JSON_Value**)NULL;
    new_obj->capacity = 0;
    new_obj->count = 0;
    return new_obj;
}

static int json_object_add(JSON_Object *object, const char *name, JSON_Value *value) {
    size_t index;
    if (object->count >= object->capacity) {
        size_t new_capacity = MAX(object->capacity * 2, STARTING_CAPACITY);
        if (new_capacity > OBJECT_MAX_CAPACITY)
            return ERROR;
        if (json_object_resize(object, new_capacity) == ERROR)
            return ERROR;
    }
    if (json_object_get_value(object, name) != NULL)
        return ERROR;
    index = object->count;
    object->names[index] = parson_strndup(name, strlen(name));
    if (!object->names[index])
        return ERROR;
    object->values[index] = value;
    object->count++;
    return SUCCESS;
}

static int json_object_resize(JSON_Object *object, size_t capacity) {
    if (try_realloc((void**)&object->names, capacity * sizeof(char*)) == ERROR)
        return ERROR;
    if (try_realloc((void**)&object->values, capacity * sizeof(JSON_Value*)) == ERROR)
        return ERROR;
    object->capacity = capacity;
    return SUCCESS;
}

static JSON_Value * json_object_nget_value(const JSON_Object *object, const char *name, size_t n) {
    size_t i, name_length;
    for (i = 0; i < json_object_get_count(object); i++) {
        name_length = strlen(object->names[i]);
        if (name_length != n)
            continue;
        if (strncmp(object->names[i], name, n) == 0)
            return object->values[i];
    }
    return NULL;
}

static void json_object_free(JSON_Object *object) {
    while(object->count--) {
        parson_free(object->names[object->count]);
        json_value_free(object->values[object->count]);
    }
    parson_free(object->names);
    parson_free(object->values);
    parson_free(object);
}

/* JSON Array */
static JSON_Array * json_array_init(void) {
    JSON_Array *new_array = (JSON_Array*)parson_malloc(sizeof(JSON_Array));
    if (!new_array)
        return NULL;
    new_array->items = (JSON_Value**)NULL;
    new_array->capacity = 0;
    new_array->count = 0;
    return new_array;
}

static int json_array_add(JSON_Array *array, JSON_Value *value) {
    if (array->count >= array->capacity) {
        size_t new_capacity = MAX(array->capacity * 2, STARTING_CAPACITY);
        if (new_capacity > ARRAY_MAX_CAPACITY)
            return ERROR;
        if (!json_array_resize(array, new_capacity))
            return ERROR;
    }
    array->items[array->count] = value;
    array->count++;
    return SUCCESS;
}

static int json_array_resize(JSON_Array *array, size_t capacity) {
    if (try_realloc((void**)&array->items, capacity * sizeof(JSON_Value*)) == ERROR)
        return ERROR;
    array->capacity = capacity;
    return SUCCESS;
}

static void json_array_free(JSON_Array *array) {
    while (array->count--)
        json_value_free(array->items[array->count]);
    parson_free(array->items);
    parson_free(array);
}

/* JSON Value */
static JSON_Value * json_value_init_object(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONObject;
    new_value->value.object = json_object_init();
    if (!new_value->value.object) {
        parson_free(new_value);
        return NULL;
    }
    return new_value;
}

static JSON_Value * json_value_init_array(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONArray;
    new_value->value.array = json_array_init();
    if (!new_value->value.array) {
        parson_free(new_value);
        return NULL;
    }
    return new_value;
}

static JSON_Value * json_value_init_string(const char *string) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONString;
    new_value->value.string = string;
    return new_value;
}

static JSON_Value * json_value_init_number(double number) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONNumber;
    new_value->value.number = number;
    return new_value;
}

static JSON_Value * json_value_init_boolean(int boolean) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONBoolean;
    new_value->value.boolean = boolean;
    return new_value;
}

static JSON_Value * json_value_init_null(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value)
        return NULL;
    new_value->type = JSONNull;
    return new_value;
}

/* Parser */
static void skip_quotes(const char **string) {
    skip_char(string);
    while (**string != '\"') {
        if (**string == '\0')
            return;
        if (**string == '\\') {
            skip_char(string);
            if (**string == '\0')
                return;
        }
        skip_char(string);
    }
    skip_char(string);
}

/* Returns contents of a string inside double quotes and parses escaped
 characters inside.
 Example: "\u006Corem ipsum" -> lorem ipsum */
static const char * get_processed_string(const char **string) {
    const char *string_start = *string;
    char *output, *processed_ptr, *unprocessed_ptr, current_char;
    unsigned int utf_val;
    skip_quotes(string);
    if (**string == '\0')
        return NULL;
    output = parson_strndup(string_start + 1, *string  - string_start - 2);
    if (!output)
        return NULL;
    processed_ptr = unprocessed_ptr = output;
    while (*unprocessed_ptr) {
        current_char = *unprocessed_ptr;
        if (current_char == '\\') {
            unprocessed_ptr++;
            current_char = *unprocessed_ptr;
            switch (current_char) {
                case '\"': case '\\': case '/': break;
                case 'b': current_char = '\b'; break;
                case 'f': current_char = '\f'; break;
                case 'n': current_char = '\n'; break;
                case 'r': current_char = '\r'; break;
                case 't': current_char = '\t'; break;
                case 'u':
                    unprocessed_ptr++;
                    if (!is_utf((const unsigned char*)unprocessed_ptr) ||
                        sscanf(unprocessed_ptr, "%4x", &utf_val) == EOF) {
                            parson_free(output);
                            return NULL;
                    }
                    if (utf_val < 0x80) {
                        current_char = utf_val;
                    } else if (utf_val < 0x800) {
                        *processed_ptr++ = (utf_val >> 6) | 0xC0;
                        current_char = ((utf_val | 0x80) & 0xBF);
                    } else {
                        *processed_ptr++ = (utf_val >> 12) | 0xE0;
                        *processed_ptr++ = (((utf_val >> 6) | 0x80) & 0xBF);
                        current_char = ((utf_val | 0x80) & 0xBF);
                    }
                    unprocessed_ptr += 3;
                    break;
                default:
                    parson_free(output);
                    return NULL;
                    break;
            }
        } else if ((unsigned char)current_char < 0x20) { /* 0x00-0x19 are invalid characters for json string (http://www.ietf.org/rfc/rfc4627.txt) */
            parson_free(output);
            return NULL;
        }
        *processed_ptr = current_char;
        processed_ptr++;
        unprocessed_ptr++;
    }
    *processed_ptr = '\0';
    if (try_realloc((void**)&output, strlen(output) + 1) == ERROR)
        return NULL;
    return output;
}

static JSON_Value * parse_value(const char **string, size_t nesting) {
    if (nesting > MAX_NESTING)
        return NULL;
    skip_whitespaces(string);
    switch (**string) {
        case '{':
            return parse_object_value(string, nesting + 1);
        case '[':
            return parse_array_value(string, nesting + 1);
        case '\"':
            return parse_string_value(string);
        case 'f': case 't':
            return parse_boolean_value(string);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number_value(string);
        case 'n':
            return parse_null_value(string);
        default:
            return NULL;
    }
}

static JSON_Value * parse_object_value(const char **string, size_t nesting) {
    JSON_Value *output_value = json_value_init_object(), *new_value = NULL;
    JSON_Object *output_object = json_value_get_object(output_value);
    const char *new_key = NULL;
    if (!output_value)
        return NULL;
    skip_char(string);
    skip_whitespaces(string);
    if (**string == '}') { /* empty object */
        skip_char(string);
        return output_value;
    }
    while (**string != '\0') {
        new_key = get_processed_string(string);
        skip_whitespaces(string);
        if (!new_key || **string != ':') {
            json_value_free(output_value);
            return NULL;
        }
        skip_char(string);
        new_value = parse_value(string, nesting);
        if (!new_value) {
            parson_free(new_key);
            json_value_free(output_value);
            return NULL;
        }
        if(!json_object_add(output_object, new_key, new_value)) {
            parson_free(new_key);
            parson_free(new_value);
            json_value_free(output_value);
            return NULL;
        }
        parson_free(new_key);
        skip_whitespaces(string);
        if (**string != ',')
            break;
        skip_char(string);
        skip_whitespaces(string);
    }
    skip_whitespaces(string);
    if (**string != '}' || /* Trim object after parsing is over */
         json_object_resize(output_object, json_object_get_count(output_object)) == ERROR) {
        json_value_free(output_value);
        return NULL;
    }
    skip_char(string);
    return output_value;
}

static JSON_Value * parse_array_value(const char **string, size_t nesting) {
    JSON_Value *output_value = json_value_init_array(), *new_array_value = NULL;
    JSON_Array *output_array = json_value_get_array(output_value);
    if (!output_value)
        return NULL;
    skip_char(string);
    skip_whitespaces(string);
    if (**string == ']') { /* empty array */
        skip_char(string);
        return output_value;
    }
    while (**string != '\0') {
        new_array_value = parse_value(string, nesting);
        if (!new_array_value) {
            json_value_free(output_value);
            return NULL;
        }
        if(json_array_add(output_array, new_array_value) == ERROR) {
            parson_free(new_array_value);
            json_value_free(output_value);
            return NULL;
        }
        skip_whitespaces(string);
        if (**string != ',')
            break;
        skip_char(string);
        skip_whitespaces(string);
    }
    skip_whitespaces(string);
    if (**string != ']' || /* Trim array after parsing is over */
         json_array_resize(output_array, json_array_get_count(output_array)) == ERROR) {
        json_value_free(output_value);
        return NULL;
    }
    skip_char(string);
    return output_value;
}

static JSON_Value * parse_string_value(const char **string) {
    const char *new_string = get_processed_string(string);
    if (!new_string)
        return NULL;
    return json_value_init_string(new_string);
}

static JSON_Value * parse_boolean_value(const char **string) {
    size_t true_token_size = sizeof_token("true");
    size_t false_token_size = sizeof_token("false");
    if (strncmp("true", *string, true_token_size) == 0) {
        *string += true_token_size;
        return json_value_init_boolean(1);
    } else if (strncmp("false", *string, false_token_size) == 0) {
        *string += false_token_size;
        return json_value_init_boolean(0);
    }
    return NULL;
}

static JSON_Value * parse_number_value(const char **string) {
    char *end;
    double number = strtod(*string, &end);
    JSON_Value *output_value;
    if (is_decimal(*string, end - *string)) {
        *string = end;
        output_value = json_value_init_number(number);
    } else {
        output_value = NULL;
    }
    return output_value;
}

static JSON_Value * parse_null_value(const char **string) {
    size_t token_size = sizeof_token("null");
    if (strncmp("null", *string, token_size) == 0) {
        *string += token_size;
        return json_value_init_null();
    }
    return NULL;
}

/* Parser API */
JSON_Value * json_parse_file(const char *filename) {
    char *file_contents = read_file(filename);
    JSON_Value *output_value = NULL;
    if (!file_contents)
        return NULL;
    output_value = json_parse_string(file_contents);
    parson_free(file_contents);
    return output_value;
}

JSON_Value * json_parse_file_with_comments(const char *filename) {
    char *file_contents = read_file(filename);
    JSON_Value *output_value = NULL;
    if (!file_contents)
        return NULL;
    output_value = json_parse_string_with_comments(file_contents);
    parson_free(file_contents);
    return output_value;
}

JSON_Value * json_parse_string(const char *string) {
    if (!string || (*string != '{' && *string != '['))
        return NULL;
    return parse_value((const char**)&string, 0);
}

JSON_Value * json_parse_string_with_comments(const char *string) {
    JSON_Value *result = NULL;
    char *string_mutable_copy = NULL, *string_mutable_copy_ptr = NULL;
    string_mutable_copy = parson_strndup(string, strlen(string));
    if (!string_mutable_copy)
        return NULL;
    remove_comments(string_mutable_copy, "/*", "*/");
    remove_comments(string_mutable_copy, "//", "\n");
    string_mutable_copy_ptr = string_mutable_copy;
    skip_whitespaces(&string_mutable_copy_ptr);
    if (*string_mutable_copy_ptr != '{' && *string_mutable_copy_ptr != '[') {
        parson_free(string_mutable_copy);
        return NULL;
    }
    result = parse_value((const char**)&string_mutable_copy_ptr, 0);
    parson_free(string_mutable_copy);
    return result;
}


/* JSON Object API */

JSON_Value * json_object_get_value(const JSON_Object *object, const char *name) {
    return json_object_nget_value(object, name, strlen(name));
}

const char * json_object_get_string(const JSON_Object *object, const char *name) {
    return json_value_get_string(json_object_get_value(object, name));
}

double json_object_get_number(const JSON_Object *object, const char *name) {
    return json_value_get_number(json_object_get_value(object, name));
}

JSON_Object * json_object_get_object(const JSON_Object *object, const char *name) {
    return json_value_get_object(json_object_get_value(object, name));
}

JSON_Array * json_object_get_array(const JSON_Object *object, const char *name) {
    return json_value_get_array(json_object_get_value(object, name));
}

int json_object_get_boolean(const JSON_Object *object, const char *name) {
    return json_value_get_boolean(json_object_get_value(object, name));
}

JSON_Value * json_object_dotget_value(const JSON_Object *object, const char *name) {
    const char *dot_position = strchr(name, '.');
    if (!dot_position)
        return json_object_get_value(object, name);
    object = json_value_get_object(json_object_nget_value(object, name, dot_position - name));
    return json_object_dotget_value(object, dot_position + 1);
}

const char * json_object_dotget_string(const JSON_Object *object, const char *name) {
    return json_value_get_string(json_object_dotget_value(object, name));
}

double json_object_dotget_number(const JSON_Object *object, const char *name) {
    return json_value_get_number(json_object_dotget_value(object, name));
}

JSON_Object * json_object_dotget_object(const JSON_Object *object, const char *name) {
    return json_value_get_object(json_object_dotget_value(object, name));
}

JSON_Array * json_object_dotget_array(const JSON_Object *object, const char *name) {
    return json_value_get_array(json_object_dotget_value(object, name));
}

int json_object_dotget_boolean(const JSON_Object *object, const char *name) {
    return json_value_get_boolean(json_object_dotget_value(object, name));
}

size_t json_object_get_count(const JSON_Object *object) {
    return object ? object->count : 0;
}

const char * json_object_get_name(const JSON_Object *object, size_t index) {
    if (index >= json_object_get_count(object))
        return NULL;
    return object->names[index];
}

/* JSON Array API */
JSON_Value * json_array_get_value(const JSON_Array *array, size_t index) {
    if (index >= json_array_get_count(array))
        return NULL;
    return array->items[index];
}

const char * json_array_get_string(const JSON_Array *array, size_t index) {
    return json_value_get_string(json_array_get_value(array, index));
}

double json_array_get_number(const JSON_Array *array, size_t index) {
    return json_value_get_number(json_array_get_value(array, index));
}

JSON_Object * json_array_get_object(const JSON_Array *array, size_t index) {
    return json_value_get_object(json_array_get_value(array, index));
}

JSON_Array * json_array_get_array(const JSON_Array *array, size_t index) {
    return json_value_get_array(json_array_get_value(array, index));
}

int json_array_get_boolean(const JSON_Array *array, size_t index) {
    return json_value_get_boolean(json_array_get_value(array, index));
}

size_t json_array_get_count(const JSON_Array *array) {
    return array ? array->count : 0;
}

/* JSON Value API */
JSON_Value_Type json_value_get_type(const JSON_Value *value) {
    return value ? value->type : JSONError;
}

JSON_Object * json_value_get_object(const JSON_Value *value) {
    return json_value_get_type(value) == JSONObject ? value->value.object : NULL;
}

JSON_Array * json_value_get_array(const JSON_Value *value) {
    return json_value_get_type(value) == JSONArray ? value->value.array : NULL;
}

const char * json_value_get_string(const JSON_Value *value) {
    return json_value_get_type(value) == JSONString ? value->value.string : NULL;
}

double json_value_get_number(const JSON_Value *value) {
    return json_value_get_type(value) == JSONNumber ? value->value.number : 0;
}

int json_value_get_boolean(const JSON_Value *value) {
    return json_value_get_type(value) == JSONBoolean ? value->value.boolean : -1;
}

void json_value_free(JSON_Value *value) {
    switch (json_value_get_type(value)) {
        case JSONObject:
            json_object_free(value->value.object);
            break;
        case JSONString:
            if (value->value.string) { parson_free(value->value.string); }
            break;
        case JSONArray:
            json_array_free(value->value.array);
            break;
        default:
            break;
    }
    parson_free(value);
}
