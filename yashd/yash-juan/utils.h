#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int strEquals(char* str1, char* str2) {
    if (strlen(str1) != strlen(str2)) {
        return 0;
    }
    for (int i = 0; i < strlen(str1); i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    return 1;
}

void printList(char* list[]) {
    int i = 0;
    while (1) {
        if (list[i] == NULL) {
            return;
        }
        printf("list[%d]='%s'", i, list[i]);
        i++;
    }
}

void clearArray(char* array[], int size) {
    for (int i = 0; i < size; i++) {
        array[i] = NULL;
    }
}

void prepend(char* s, const char* t) {
    // https://stackoverflow.com/a/2328191
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}

char* removeFirst(char* s, int i) {
    return s + i;
}