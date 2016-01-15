#include <string.h>
#include <stdlib.h>

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    if (!result) return result; // return the unsuccessful NULL
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
