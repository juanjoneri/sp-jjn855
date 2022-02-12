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
    char temp[strlen(source) + 1];
    strcpy(temp, source);
    temp[group.rm_eo] = 0;
    char* value = temp + group.rm_so;
    return copyString(value);
}