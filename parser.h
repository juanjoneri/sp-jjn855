#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

struct command {
    char* program;
    char* arguments[50];
    int argument_count;
    int background;
    char* outfile;
    char* infile;
    struct command* pipe;
};

struct command* allocCommand() {
    struct command* c = malloc(sizeof(struct command));
    c->program = NULL;
    c->argument_count = 0;
    c->background = 0;
    c->outfile = NULL;
    c->infile = NULL;
    c->pipe = NULL;
    return c;
}

void freeCommand(struct command* c) {
    free(c->program);
    if (c->outfile != NULL) {
        free(c->outfile);
    }
    if (c->infile != NULL) {
        free(c->infile);
    }
    for (int i = 0; i < c->argument_count; i++) {
        free(c->arguments[i]);
    }
    if (c->pipe != NULL) {
        freeCommand(c->pipe);
    }
    free(c);
}

void printCommand(struct command* c) {
    for (int i = 0; i < c->argument_count; i++) {
        printf("%s ", c->arguments[i]);
    }
    // input redirection
    if (c->infile != NULL) {
        printf("< %s ", c->infile);
    }
    // output redirection
    if (c->outfile != NULL) {
        printf("> %s ", c->outfile);
    }
    if (c->background) {
        // TODO: skip & when priting from fg
        printf("& ");
    }
    if (c->pipe != NULL) {
        printf("| ");
        printCommand(c->pipe);
    }
}

void addPipe(struct command* left, struct command* right) {
    left->pipe = right;
}

void addArgument(struct command* c, char* argument) {
    c->arguments[c->argument_count] = copyString(argument);
    c->argument_count++;
}

void setProgram(struct command* c, char* program) {
    c->program = copyString(program);
    addArgument(c, program);
}

void setBackground(struct command* c) { c->background = 1; }

void setInfile(struct command* c, char* file) { c->infile = copyString(file); }

void setOutfile(struct command* c, char* file) {
    c->outfile = copyString(file);
}

void addArguments(struct command* c, char* arguments) {
    // TODO: Text within quotes should be a single argument
    char* pch = strtok(arguments, " ");
    while (pch != NULL) {
        addArgument(c, pch);
        pch = strtok(NULL, " ");
    }
}

struct command* parseCommand(char* source) {
    // program arg1 arg2 < infile.txt > outfile.txt &
    char* command_pat =
        "([^|<>& ]+)([^|<>&]+)?\\s*([<>]\\s*[^|<>& ]+)?\\s*([<>]\\s*[^|<>& "
        "]+)?\\s?(&)?";
    size_t max_groups = 6;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, command_pat, REG_EXTENDED);

    struct command* c = allocCommand();

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
        for (int g = 0; g < max_groups; g++) {
            if (groups[g].rm_so == -1) {
                break;  // No more groups
            }
            if (g == 0) {
                continue;  // skip full match
            }

            char* value = extractGroup(source, groups[g]);
            if (g == 1) {
                setProgram(c, value);
            } else if (startsWith(value, "&")) {
                setBackground(c);
            } else if (startsWith(value, ">")) {
                value = removeLeadingWhitespace(++value);
                setOutfile(c, value);
            } else if (startsWith(value, "<")) {
                value = removeLeadingWhitespace(++value);
                setInfile(c, value);
            } else {
                addArguments(c, value);
            }
        }
    }

    regfree(&regex);
    return c;
}

struct command* parsePipe(char* source) {
    char* pipe_pat = "([^|]+)\\s*\\|\\s*([^|]+)";
    size_t max_groups = 3;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, pipe_pat, REG_EXTENDED);

    struct command* c;

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
        for (int g = 0; g < max_groups; g++) {
            if (groups[g].rm_so == -1) {
                break;  // No more groups
            }
            if (g == 0) {
                continue;  // skip full match
            }

            char* value = extractGroup(source, groups[g]);
            if (g == 1) {
                // left side of tye pipe
                c = parseCommand(value);
            } else {
                // right side of the pipe
                addPipe(c, parseCommand(value));
            }
        }

    } else {
        // input did not match regex therefore we have no pipe
        c = parseCommand(source);
    }
    regfree(&regex);
    return c;
}