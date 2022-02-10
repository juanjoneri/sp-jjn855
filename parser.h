#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* copyString(char* str) {
  char *copy = malloc(strlen(str) + 1);
  strcpy(copy, str);
  return copy;
}

struct command {
  char* program;
  char* arguments[50];
  int argument_count;
};

struct command* allocCommand() {
  struct command* c = malloc(sizeof(struct command));
  c->argument_count = 0;
  return c;
}

void freeCommand(struct command* c) {
  free(c->program);
  for (int i = 0; i < c->argument_count; i++) {
    free(c->arguments[i]);
  }
  free(c);
}

void setProgram(struct command* c, char* program) {
  c->program = copyString(program);
}

void addArgument(struct command* c, char* argument) {
  c->arguments[c->argument_count] = copyString(argument);
  c->argument_count++;
}

void addArguments(struct command* c, char* arguments) {
  char* pch = strtok(arguments," ");
  while (pch != NULL) {
    addArgument(c, pch);
    pch = strtok (NULL, " ");
  }
}

struct command* parseCommand(char* source) {    
    char* command_pat = "(\\w+)([-a-z0-9 ]*)";
    size_t max_groups = 3;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, command_pat, REG_EXTENDED);

    struct command* c = allocCommand();

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
      for (int g = 0; g < max_groups; g++){
          if (groups[g].rm_so == -1) {
            break;  // No more groups
          }
          if (g == 0) {
            continue; // skip full match
          }
          char group[strlen(source) + 1];
          strcpy(group, source);
          group[groups[g].rm_eo] = 0;
          char* value = group + groups[g].rm_so;
          if (g == 1) {
            setProgram(c, value);
          } else {
            addArguments(c, value);
          }
        }
    }

    regfree(&regex);
    return c;

}