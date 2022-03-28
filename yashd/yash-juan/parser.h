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
    clearArray(c->arguments, 50);
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

int hasOutputRedirection(struct command* c) { return c->outfile != NULL; }

int hasInputRedirection(struct command* c) { return c->infile != NULL; }

int isBackgroundJob(struct command* c) { return c->background; }

int hasPipe(struct command* c) { return c->pipe != NULL; }

void printCommand(struct command* c, FILE *log_fd) {
    fprintf(log_fd,"%s", c->program);
    for (int i = 1; i < c->argument_count; i++) {
        fprintf(log_fd," %s", c->arguments[i]);
    }
    if (hasInputRedirection(c)) {
        fprintf(log_fd," < %s", c->infile);
    }
    if (hasOutputRedirection(c)) {
        fprintf(log_fd," > %s", c->outfile);
    }
    if (isBackgroundJob(c)) {
        // TODO: skip & when priting from fg
        fprintf(log_fd," &");
    }
    if (hasPipe(c)) {
        fprintf(log_fd," | ");
        printCommand(c->pipe, log_fd);
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
        "]+)?\\s*(&)?\\s*";
    size_t max_groups = 6;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, command_pat, REG_EXTENDED | REG_NEWLINE);

    struct command* c = allocCommand();

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
        for (int g = 0; g < max_groups; g++) {
            if (g == 0 || groups[g].rm_so == -1) {
                continue;  // Skip full match and empty groups
            }
            char* value = extractGroup(source, groups[g]);
            if (g == 1) {
                setProgram(c, value);
            } else if (startsWith(value, "&")) {
                setBackground(c);
            } else if (startsWith(value, ">")) {
                setOutfile(c, removeLeadingWhitespace(++value));
            } else if (startsWith(value, "<")) {
                setInfile(c, removeLeadingWhitespace(++value));
            } else {
                addArguments(c, value);
            }
        }
    }

    regfree(&regex);
    return c;
}

struct command* parsePipe(char* source) {
    char* pipe_pat = "([^|]+)\\|([^|]+)";
    size_t max_groups = 3;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, pipe_pat, REG_EXTENDED);

    struct command* c;

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
        for (int g = 0; g < max_groups; g++) {
            if (g == 0 || groups[g].rm_so == -1) {
                continue;  // Skip full match and empty groups
            }

            char* value = extractGroup(source, groups[g]);
            if (g == 1) {
                // left side of tye pipe
                c = parseCommand(value);
            } else {
                // right side of the pipe
                addPipe(c, parseCommand(value));
            }
            free(value);
        }

    } else {
        // input did not match regex therefore we have no pipe
        c = parseCommand(source);
    }
    regfree(&regex);
    return c;
}