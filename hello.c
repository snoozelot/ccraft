#!/usr/bin/env -S ccraft -lm -vv
// hello - demonstrate argument parsing and greeting
//
// Prints all command-line arguments, environment variables, then greets
// the first argument or "world" if no arguments provided.
//
// Usage:
//   ./hello [NAME]
//
// Examples:
//   ./hello        # prints "hello world!"
//   ./hello Alice  # prints "hello Alice!"

#include <stdio.h>
#include <string.h>

static void
print_arguments(int argc, char *const argv[]) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
}

static void
print_environment(char *envp[]) {
    while (*envp) {
        char *equal_pos = strchr(*envp, '=');
        *equal_pos = '\0';
        printf("envp %s = %s\n", *envp, equal_pos + 1);
        *equal_pos = '=';
        envp++;
    }
}

int
main(int argc, char *argv[], char *envp[]) {
    printf("\n");
    printf("argc = %d\n", argc);
    print_arguments(argc, argv);

 // printf("\n");
 // print_environment(envp);

    printf("\n");
    printf("hello %s!\n", argc > 1 ? argv[1] : "world");

    return 0;
}
