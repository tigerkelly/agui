/*
 * trimstr.c - remove leading and trailing whitespace from a string
 *
 * Provides two functions:
 *   trimstr(s)       - trims in-place, returns pointer to s
 *   trimstr_alloc(s) - returns a newly malloc'd trimmed copy (caller must free)
 */

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * trimstr - trim leading and trailing whitespace in-place.
 *
 * The string is modified directly:
 *   - Leading whitespace is removed by shifting characters left.
 *   - Trailing whitespace is removed by writing a NUL terminator.
 *
 * Returns s.
 */
char *trimstr(char *s) {
    if (!s) return s;

    /* find first non-whitespace character */
    char *start = s;
    while (*start && isspace((unsigned char)*start))
        start++;

    /* shift left over leading whitespace */
    if (start != s)
        memmove(s, start, strlen(start) + 1);  /* +1 copies the NUL */

    /* trim trailing whitespace */
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1)))
        end--;
    *end = '\0';

    return s;
}

/*
 * trimstr_alloc - return a malloc'd copy of s with whitespace trimmed.
 *
 * The original string is not modified.
 * Returns NULL if s is NULL or memory allocation fails.
 * The caller is responsible for freeing the returned pointer.
 */
char *trimstr_alloc(const char *s) {
    if (!s) return NULL;

    /* skip leading whitespace */
    while (*s && isspace((unsigned char)*s))
        s++;

    /* find end, then walk back over trailing whitespace */
    const char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1)))
        end--;

    size_t len = (size_t)(end - s);
    char *result = malloc(len + 1);
    if (!result) return NULL;

    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}
