```
$ gcc -o yash yash.c prompt.h parser.h utils.h -lreadline
$ ./yash
# ls
README.md  infloop  infloop.c  job.c  parser.h  prompt.h  test.c  utils.h  yash  yash.c
# ls | wc
     10      10      79
# cat < infloop.c
int main() {
    while(1) {}
}
# history
 0 ls
 1 ls | wc
 2 cat < infloop.c
#
```
