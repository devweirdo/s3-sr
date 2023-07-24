#include <stdio.h>
#include <string.h>

char* strip(char* str) {
    // Find the first non-whitespace character in the string
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t' || str[start] == '\n') {
        start++;
    }
    // Find the last non-whitespace character in the string
    int end = strlen(str) - 1;
    while (str[end] == ' ' || str[end] == '\t' || str[end] == '\n') {
        end--;
    }
    // Copy the substring between start and end to a new string
    int len       = end - start + 1;
    char* new_str = malloc(len + 1);
    strncpy(new_str, str + start, len);
    new_str[len] = '\0';
    // Return the new string
    return new_str;
}

void sr_detect_action(int cmd_id, int phr_id, char* string, float prob) {
    char* str = strip(string);

    printf("%s\n", str);

    if (!strcmp(str, "HELLO")) {
        printf("Hi! I'm esp\n");
    } else if (!strcmp(str, "WHO ARE YOU")) {
        printf("I'm esp\n");
    } else if (!strcmp(str, "TELL ME A JOKE")) {
        printf("You are the joke\n");
    }

    free(str);
    str = NULL;
}