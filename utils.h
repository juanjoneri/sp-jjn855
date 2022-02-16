#include <regex.h>
#include <stdlib.h>
#include <string.h>

char* copyString(char* str) {
    char* copy = malloc(strlen(str) + 1);
    strcpy(copy, str);
    return copy;
}

char* removeLeadingWhitespace(char* str) {
    while (str[0] == 32) {
        str++;
    }
    return copyString(str);
}

int startsWith(char* str, char* c) { return strncmp(c, str, strlen(c)) == 0; }

char* extractGroup(char* source, regmatch_t group) {
    char* result = malloc(group.rm_eo - group.rm_so + 1);
    int c = 0;
    for (int i = group.rm_so; i < group.rm_eo; i++) {
        result[c] = source[i];
        c++;
    }
    result[c] = '\0';
    return result;
}