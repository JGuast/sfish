#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

struct __attribute__((__packed__))process {
	struct process* prev;
	struct process* next;
	int JID;
	char status[8]; // STOPPED/RUNNING each 7 chars + 1 for '\0'
	pid_t PID; // can be printed by casting to long int and using %ld
	char name[50];
};
typedef struct process Proc;

void print_help();

void handle_chpmt(char**);

void handle_chclr(char**);

void handle_cd(char**);

void handle_redirect(char**, int, char*);

void rebuild_prompt();

int is_builtin(char*, int);

void exec_builtin(char*, int, char**);

void exec_builtin_plus(char*);

void reap_list(int);