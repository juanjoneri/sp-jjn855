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
  char* source; // Value as entered by end user
  char* program;
  char* arguments[50];
  int argument_count;
  int background;
  char* outfile;
  char* infile;
  struct command* pipe;
};

struct command* allocCommand(char* source) {
  struct command* c = malloc(sizeof(struct command));
  c->source = copyString(source);
  c->argument_count = 0;
  c->background = 0;
  return c;
}

void freeCommand(struct command* c) {
  free(c->source);
  free(c->program);
  for (int i = 0; i < c->argument_count; i++) {
    free(c->arguments[i]);
  }
  if(c->pipe != NULL) {
    freeCommand(c->pipe);
  }
  free(c);
}

void printCommand(struct command* c) {
  for (int i = 0; i < c->argument_count; i++) {
    printf("%s ", c->arguments[i]);
  }
  if (c->pipe != NULL) {
    printf("| ");
    printCommand(c->pipe);
  }
  // input redirection
  // output redirection
  if (c->background) {
    // TODO: skip & when priting from fg
    printf("&");
  }
  printf("\n");
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

void setBackground(struct command* c) {
  c->background = 1;
}

void addArguments(struct command* c, char* arguments) {
  // TODO: Text within quotes should be a single argument
  char* pch = strtok(arguments," ");
  while (pch != NULL) {
    addArgument(c, pch);
    pch = strtok (NULL, " ");
  }
}

char* extractGroup(char* source, regmatch_t group) {
    char temp[strlen(source) + 1];
    strcpy(temp, source);
    temp[group.rm_eo] = 0;
    char* value = temp + group.rm_so;
    return copyString(value);
}

struct command* parseCommand(char* source) {    
    char* command_pat = "(\\w+)([-A-Za-z0-9 ]*)(\\s?&)?";
    size_t max_groups = 4;

    regex_t regex;
    regmatch_t groups[max_groups];

    regcomp(&regex, command_pat, REG_EXTENDED);

    struct command* c = allocCommand(source);

    if (regexec(&regex, source, max_groups, groups, 0) == 0) {
      for (int g = 0; g < max_groups; g++){
          if (groups[g].rm_so == -1) {
            break;  // No more groups
          }
          if (g == 0) {
            continue; // skip full match
          }

          char* value = extractGroup(source, groups[g]);
          if (g == 1) {
            setProgram(c, value);
          } else if (g == 2) {
            addArguments(c, value);
          } else {
            setBackground(c);
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
      for (int g = 0; g < max_groups; g++){
          if (groups[g].rm_so == -1) {
            break;  // No more groups
          }
          if (g == 0) {
            continue; // skip full match
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