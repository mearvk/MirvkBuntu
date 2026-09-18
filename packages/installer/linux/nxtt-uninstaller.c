#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void menu(void) {
    puts("\033[38;5;208mnxtt — Ubuntu White Edition Uninstaller\033[0m");
    puts("\033[38;5;208m-----------------------------------------\033[0m");
    puts("[M] Main      Core components and dependencies");
    puts("[I] Installed User-installed software");
    puts("[S] Sibling   Related software and optional companions");
    puts("");
    puts("Arrow keys: select / deselect software");
    puts("Space:      proceed with the designated action");
    puts("Space Space: acknowledge risk, then stop for review");
    puts("Ctrl+Enter: show dependency/relationship breakdown");
    puts("Q:          stop and return control to the user");
}

int main(void) {
    menu();
    puts("");
    puts("No software will be removed by this preview build.");
    puts("The production uninstaller will require explicit confirmation before deletion.");
    return EXIT_SUCCESS;
}
