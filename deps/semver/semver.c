// Copyright (c) 2014 Eddy Luten - https://github.com/EddyLuten/lib-semver
// Distributed under MIT license.
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <semver/semver.h>

// Custom allocators, default to the defaults.
static semver_malloc_fnc semver_malloc = (semver_malloc_fnc)malloc;
static semver_realloc_fnc semver_realloc = (semver_realloc_fnc)realloc;
static semver_free_fnc semver_free = (semver_free_fnc)free;
static FILE* semver_out = NULL;

//INTERNAL
#ifdef NDEBUG
#define print_debug(ignore) ((void)0)
#else
void print_debug(const char* format, ...)
{
  va_list list;

  if (NULL == semver_out)
    return;

  va_start(list, format);
  vfprintf(semver_out, format, list);
  va_end(list);
}
#endif

// INTERNAL
// Extracts the pre-release information from the provided string.
// begin - pointer to the beginning of the string
// end - pointer to the end of the string or NULL to use the entirety of begin.
// result - pointer to an array of pointers to hold the pre-release components.
// returns the amount of pre-release components parsed, 0 on failure.
uint64_t pre_release_set(const char* begin, const char* end, char*** result)
{
  size_t copy_length,
         result_size;
  char delimiters[2] = { '.', '\0' };
  char* token;
  char* input_copy;
  char* element;

  // Sanity check
  if (NULL == begin || (end < begin && NULL != end))
    return 0;

  // Create a copy of the whole string
  copy_length = (size_t)(strchr(begin, NULL == end ? '\0' : *end) - begin);
  input_copy = semver_malloc((copy_length + 1) * sizeof(char));

  if (NULL == input_copy)
    return 0;

  memcpy(input_copy, begin, copy_length * sizeof(char));
  input_copy[copy_length] = '\0';

  // Tokenize the string
  token = strtok(input_copy, delimiters);
  result_size = 0;
  while (NULL != token)
  {
    // Copy the token into a newly allocated string
    element = semver_malloc((strlen(token) + 1) * sizeof(char));
    assert(NULL != element);
    if (NULL == element)
    {
      semver_free(input_copy);
      return 0;
    }

    strcpy(element, token);

    // Allocate new memory for the new array element
    (*result) = semver_realloc((*result), ++result_size * sizeof(char*));

    assert(NULL != (*result));
    if (NULL == *result)
    {
      semver_free(element);
      semver_free(input_copy);
      return 0;
    }

    // "Insert" the new item in the array
    (*result)[result_size - 1] = element;

    // Advance the token
    token = strtok(NULL, delimiters);
  }

  semver_free(input_copy);
  return result_size;
}

// INTERNAL
// Frees the memory allocated by pre-release information in a version stuct.
// pre_release - pointer to an array of pointers to be freed.
// element_count - the amount of elements in pre_release.
void pre_release_unset(char*** pre_release, const size_t element_count)
{ 
  size_t i;

  if (NULL == pre_release)
    return;

  for (i = 0; i < element_count; ++i)
  {
    if (NULL != (*pre_release)[i])
      semver_free((*pre_release)[i]);
  }

  semver_free(*pre_release);
}

// INTERNAL
// Validates the pre-release and build-info labels
bool validate_label(const char* label)
{
  char c;
  size_t i, label_length = strlen(label);
  for (i = 0; i < label_length; ++i)
  {
    c = label[i];
    if (!( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') || c == '.' || c == '-' ))
      return false;
  }

  return true;
}

// INTERNAL
// Validates the contents of each pre_release element.
bool pre_release_validate(SemVer* version)
{
  size_t i;

  // Verify that there is someting to validate
  if (!version->pre_release || 0 == version->pre_release_elements)
    return true;

  for (i = 0; i < version->pre_release_elements; ++i)
    if (!validate_label(version->pre_release[i]))
      return false;

  return true;
}

// INTERNAL
// Validates the build information.
bool build_info_validate(SemVer* version)
{
  // If no build info is available, skip.
  if (!version->build_info)
    return true;
  return validate_label(version->build_info);
}

/**
 * Allows the usage of custom memory allocators. If not provided, the library
 * will use the default malloc(), realloc(), and free() implementations.
 */
void semver_config(semver_malloc_fnc custom_malloc,
                   semver_realloc_fnc custom_realloc,
                   semver_free_fnc custom_free,
                   FILE* debug_out)
{
  semver_malloc = NULL == custom_malloc ? semver_malloc : custom_malloc;
  semver_realloc = NULL == custom_realloc ? semver_realloc : custom_realloc;
  semver_free = NULL == custom_free ? semver_free : custom_free;
  semver_out = debug_out;
}

SemVer* semver_parse(const char* string)
{
  SemVer* version;
  uint64_t major = 0, minor = 0, patch = 0;
  size_t build_info_length;
  char* pre_release_pos;
  char* build_info_pos;
  char last_char = 0;

  if (NULL == string)
  {
    print_debug("semver_parse: argument is NULL\n");
    return NULL;
  }

  pre_release_pos = strchr(string, '-');
  build_info_pos = strchr(string, '+');

  // Since it's valid for the build info to contain dashes, make sure that the
  // pre-release position is valid.
  if (NULL != build_info_pos && pre_release_pos > build_info_pos)
    pre_release_pos = NULL;

  // Scan for the version information that should always be present.
  if (sscanf(string,
             "%llu.%llu.%llu%c",
             &major,
             &minor,
             &patch,
             &last_char) < 3)
  {
    print_debug("semver_parse: string format not allowed\n");
    return NULL;
  }

  version = semver_create(major, minor, patch);
  if (NULL == version)
    return NULL;

  // Make sure that the character after the required components is one of the
  // allowed characters (-,+,\0).
  switch (last_char)
  {
  case '-': case '+': case '\0':
    break;
  default:
    {
      print_debug("semver_parse: illegal characters found in string\n");
      semver_destroy(version);
      return NULL;
    }
  }

  // Scan for the pre-release information that _can_ be present.
  if (NULL != pre_release_pos && '\0' != ++pre_release_pos)
  {
    version->pre_release_elements =
      pre_release_set(pre_release_pos, build_info_pos, &version->pre_release);

    // 0 is returned when something went wrong parsing.
    if (0 == version->pre_release_elements)
    {
      print_debug("semver_parse: error parsing pre-release string\n");
      semver_destroy(version);
      return NULL;
    }
  }

  // Make a copy of the build info string.
  if (NULL != build_info_pos && '\0' != ++build_info_pos)
  {
    build_info_length = strlen(build_info_pos);
    version->build_info = semver_malloc((build_info_length + 1) * sizeof(char));
    assert(NULL != version->build_info);
    if (NULL == version->build_info)
    {
      semver_destroy(version);
      return NULL;
    }

    strcpy(version->build_info, build_info_pos);
  }

  // Validate the pre-release and build information.
  if (!pre_release_validate(version) || !build_info_validate(version))
  {
    print_debug("semver_parse: meta information badly formatted\n");
    semver_destroy(version);
    return NULL;
  }

  return version;
}

SemVer* semver_create(uint64_t major, uint64_t minor, uint64_t patch)
{
  SemVer* version;
  if (0 == major + minor + patch)
  {
    print_debug("semver_create: all arguments were zero\n");
    return NULL;
  }

  version = semver_malloc(sizeof(SemVer));
  assert(NULL != version);
  if (NULL == version)
    return NULL;

  version->major = major;
  version->minor = minor;
  version->patch = patch;
  version->pre_release = NULL;
  version->pre_release_elements = 0;
  version->build_info = NULL;

  return version;
}

bool semver_add_pre_release_component(SemVer** version,
                                      const char* component)
{
  char** temp;
  uint64_t elements;

  assert(NULL != version);
  assert(NULL != *version);
  assert(NULL != component);
  if (NULL == version || NULL == *version || NULL == component)
  {
    print_debug("semver_add_pre_release_component: bad argument(s)\n");
    return false;
  }

  // Attempt to parse the provided string.
  temp = semver_malloc(sizeof(char*));
  elements = pre_release_set(component, NULL, &temp);
  if (0 == elements)
  {
    print_debug("semver_add_pre_release_component: malformatted string\n");
    return false;
  }

  // If the pre-release info was set, override it.
  if (0 != (*version)->pre_release_elements)
    pre_release_unset(&(*version)->pre_release,
                      (*version)->pre_release_elements);

  (*version)->pre_release = temp;
  (*version)->pre_release_elements = elements;

  return true;
}

bool semver_add_build_info_component(SemVer** version,
                                    const char* restrict component)
{
  size_t length;

  assert(NULL != version);
  assert(NULL != *version);
  assert(NULL != component);
  if (NULL == version || NULL == *version ||
      NULL == component || !validate_label(component))
  {
    print_debug("semver_add_build_info_component: bad argument(s)\n");
    return false;
  }

  length = strlen(component);
  (*version)->build_info = semver_realloc((*version)->build_info,
                                          (length + 1) * sizeof(char));
  strncpy((*version)->build_info, component, length);
  (*version)->build_info[length] = '\0';

  return true;
}

void semver_destroy(SemVer* version)
{
  if (NULL == version)
    return;
  
  if (NULL != version->pre_release)
    pre_release_unset(&version->pre_release, version->pre_release_elements);

  if (NULL != version->build_info)
    semver_free(version->build_info);
  
  semver_free(version);
}

char* semver_to_string(SemVer* version)
{
  uint64_t length = 0,
           i = 0,
           j = 0;
  char* result;
  size_t result_size;
  int sprintf_result;

  // max uint64_t value "18446744073709551615" plus NULL terminator:
  char buffer[21];

  if (NULL == version)
  {
    print_debug("semver_to_string: argument is NULL\n");
    return NULL;
  }

  // If we have pre-release elements, get their lengths.
  if (NULL != version->pre_release)
  {
    ++length; // + 1 for the '-'
    for (; i < version->pre_release_elements; ++i)
      length += strlen(version->pre_release[i]) +
        (i == (version->pre_release_elements - 1) ? 0 : 1); // + 1 for the '.'
  }

  // If we have a build string, add its length as well.
  if (NULL != version->build_info)
    length += strlen(version->build_info) + 1; // + 1 for the '+'

  // Get the lengths of the required components.
  sprintf_result = snprintf(buffer, sizeof(buffer), "%llu", version->major);
  if (sprintf_result < 0)
  {
    print_debug("semver_to_string: error parsing major version\n");
    return NULL;
  }
  length += sprintf_result + 1; // + 1 for the dot separator

  sprintf_result = snprintf(buffer, sizeof(buffer), "%llu", version->minor);
  if (sprintf_result < 0)
  {
    print_debug("semver_to_string: error parsing minor version\n");
    return NULL;
  }
  length += sprintf_result + 1; // + 1 for the dot separator

  sprintf_result = snprintf(buffer, sizeof(buffer), "%llu", version->patch);
  if (sprintf_result < 0)
  {
    print_debug("semver_to_string: error parsing patch version\n");
    return NULL;
  }
  length += sprintf_result;

  // Allocate the string and begin writing.
  result_size = (length + 1) * sizeof(char);
  result = semver_malloc(result_size);
  assert(NULL != result);
  if (NULL == result)
    return NULL;

  sprintf_result = snprintf(
    result,
    result_size,
    "%llu.%llu.%llu",
    version->major,
    version->minor,
    version->patch
  );

  if (sprintf_result < 0)
  {
    print_debug("semver_to_string: error reading required components\n");
    return NULL;
  }

  j = sprintf_result;

  // If we have pre-release info, write it, too.
  if (NULL != version->pre_release)
  {
    result[j++] = '-';
    for (i = 0; i < version->pre_release_elements; ++i)
      j += snprintf(
        &result[j],
        result_size - j,
        "%s%s",
        version->pre_release[i],
        (i == (version->pre_release_elements - 1) ? "" : ".")
      );
  }

  // If we have build info, append it.
  if (NULL != version->build_info)
  {
    result[j++] = '+';
    strcpy(&result[j], version->build_info);
    j += strlen(version->build_info);
  }

  result[j] = '\0';
  return result;
}

int semver_compare(SemVer* a, SemVer* b)
{
  int compare,
      maj_cmp   = a->major - b->major,
      min_cmp   = a->minor - b->minor,
      patch_cmp = a->patch - b->patch;
  size_t i = 0,
         elements = 0;

  // Compare all of the required components.
  compare =
    0 != maj_cmp ? maj_cmp :
    0 != min_cmp ? min_cmp :
    0 != patch_cmp ? patch_cmp : 0;

  // If one of the versions has a pre-release component, we can determine which
  // one is the greater.
  if (0 == compare)
  {
    if (0 == a->pre_release_elements && 0 != b->pre_release_elements)
      return 1;
    else if (0 != a->pre_release_elements && 0 == b->pre_release_elements)
      return -1;
  }

  // Determine the max elements to use in the loop:
  elements =
    a->pre_release_elements < b->pre_release_elements
    ? a->pre_release_elements
    : b->pre_release_elements;

  // Both have elements, so iterate until one runs out of elements.
  if (0 == compare && 0 != elements)
  {
    do
      compare = strcmp(a->pre_release[i], b->pre_release[i]);
    while (0 == compare && ++i < elements);

    // Make sure that if one version has more elements than the other, the extra
    // components are taken into account as well.
    if (0 == compare && a->pre_release_elements > b->pre_release_elements)
      compare = 1; // if A has more elements than B at this point, A is greater.
    else if (0 == compare && b->pre_release_elements > a->pre_release_elements)
      compare = -1; // If B has more elements than A at this point, A is less.
  }

  return compare;
}

int semver_is_stable(SemVer* version)
{
  return version->major > 0 && NULL == version->pre_release;
}
