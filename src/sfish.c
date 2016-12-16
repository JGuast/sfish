#include "sfish.h"
 
#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define CYAN    "\x1b[36m"
#define MAGENTA "\x1b[35m"
#define BLACK   "\x1b[30m" // same as code for not bold
#define WHITE   "\x1b[37m"

#define BOLD    "\x1b[1m"
#define NOTBOLD "\x1b[0m" // same as code for black

char prompt[4000]; // initialize the prompt
int uflag;
int mflag;
char* ucolor = RED;
char* mcolor = BLUE;
char* ubold = BOLD;
char* mbold = BOLD;
int ret_value = 2121;
char cwd[1024];
char prev_dir[1024];
//getcwd(cwd, sizeof(cwd));    // getcwd
Proc* list_head;
int job_count;

int main(int argc, char** argv) 
{
    strcpy(prev_dir, "NOT SET");
    list_head = NULL;
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.

    char *cmd;
    uflag = 1;
    mflag = 1;
    job_count = 0;
    int kill_me = 0;

    char* username = getenv("USER");
    char machinename[1024];
    gethostname(machinename, 1023);
    
    strcat(prompt, WHITE);        // set color to white
    strcat(prompt, "sfish-");     // sfish-
    strcat(prompt, ucolor);       // set user color    
    strcat(prompt, username);     // user
    strcat(prompt, WHITE);        // set color to white
    strcat(prompt, "@");          // @
    strcat(prompt, mcolor);       // set machine color
    strcat(prompt, machinename);  // machine
    strcat(prompt, WHITE);        // set color to white
    strcat(prompt, ":[");         // :[
    
    //char cwd[1024];
    getcwd(cwd, sizeof(cwd)); // getcwd

    if (strcmp(cwd, "/home")==0) // if = home
        strcat(prompt, "~"); // ~
    else // else
        strcat(prompt, (cwd+5)); // cwd + 5 (/home)

    strcat(prompt, "]>");         // ]>
    //printf("The pid at the start is %ld\n", (long int)getpid());
// ================================================ MAIN WHILE LOOP ======================================================
    while((cmd = readline(prompt)) != NULL) // "sfish> " -------- readline
    { 
        reap_list(-1);
        //printf("The pid at loop start is %ld\n", (long int)getpid());
        // count occurrences of | in cmd
        int occurs = 0;
        int and_flag = 0;
        for (int i=0;i<strlen(cmd);i++)  
        {
            if (cmd[i] == '|')
            {
                occurs++;
            }
            else if (cmd[i] == '&' && and_flag == 0)
                and_flag = i;
        }

        if (and_flag > 0) // handle '&' in arguments
        {
            // handle code for this shit
            cmd[and_flag] = '\0'; // replace & with '\0'

            pid_t and_pid = fork(); // fork()
            if (and_pid != 0) // if parent
            {
                Proc* new_p = malloc(sizeof(Proc)); // declare new Proc
            
                // initialize new process with pid = and_pid
                new_p->JID = ++job_count;
                new_p->PID = and_pid;
                strcpy(new_p->name, cmd);
                strcpy(new_p->status, "Running"); // default to running

                // add it to the list
                if (list_head == NULL) // head doesn't exist
                {
                    new_p->prev = NULL;
                    new_p->next = NULL;
                    list_head = new_p;
                }
                else
                {
                    Proc* cursor = list_head;
                    while(cursor != NULL)
                    {
                        if (cursor->next == NULL)
                        {
                            cursor->next = new_p;
                            new_p->prev = cursor;
                            new_p->next = NULL;
                            break;
                        }
                        cursor = cursor->next;
                    }
                }
                // print the new job info being added to the list
                printf("[%d]\t%s\t%ld\t%s\n", new_p->JID, new_p->status, (long int)new_p->PID, new_p->name);
                continue;
            } 
            else 
                kill_me = 1; // flag child for death
        }
        
        if (occurs > 0) // if | occurs more than 0 times NEED TO PIPE
        { 
            pid_t control_pid;
            if (kill_me != 1) 
                control_pid = fork();
            else 
                control_pid = getpid();

            if (control_pid == 0)
            {
            // split into array on |, argsg
            char** argsg = NULL;
            char* p = strtok(cmd, "|");
            int argsn = 0; 

            // split string and append tokens to 'argsg' 
            while (p) 
            {
                argsg = realloc(argsg, (sizeof(char*) * ++argsn));
                if (argsg == NULL)
                    exit (-1); // memory allocation failed 

                argsg[argsn-1] = p;
                p = strtok(NULL, "|");
            }

            // realloc one extra element for the last NULL 
            argsg = realloc(argsg, sizeof(char*) * (argsn+1));
            argsg[argsn] = 0; 

            int i; 
            int in = 0; //first process should get input from original fd, 0
            int fd[2];

            for (i = 0; i < argsn-1; i++) // for each element(i) in array
            {
                pipe(fd); // pipe() the output
                                /* ----- BEGIN spawn_proc ----- */
                pid_t pid = fork();
                if (pid == 0)
                {
                    if (in != 0)
                    {
                        dup2(in, 0);
                        close(in);
                    }
                    if (fd[1] != 1)
                    {
                        dup2(fd[1], 1);
                        close(fd[1]);
                    }
                    //SPLIT S 1                  

                    // break cmd into array of strings, argsg
                    char** fuck = NULL;
                    char* p = strtok(argsg[i], " ");
                    int argsn = 0; 

                    // split string and append tokens to 'argsg' 
                    while (p) 
                    {
                        fuck = realloc(fuck, (sizeof(char*) * ++argsn));
                        if (fuck == NULL)
                            exit(-1); // memory allocation failed 

                        fuck[argsn-1] = p;
                        p = strtok(NULL, " ");
                    }

                    // realloc one extra element for the last NULL 
                    fuck = realloc(fuck, sizeof(char*) * (argsn+1));
                    fuck[argsn] = 0;

                        //SPLIT E 1
                        handle_redirect(fuck, argsn, fuck[0]);
                        //execvp(fuck[0], fuck);
                        exit(ret_value);
                    }
                                    /* ----- END spawn_proc ----- */
                    close(fd[1]); // don't need write end of pipe, child will write here 
                    in = fd[0];   // keep the in end of the pipe, next child read from there   
                } // END OF PIPE LOOP

                if (in != 0)
                    dup2(in, 0);

                cmd = argsg[i];
                argsn = strlen(cmd);
                //SPLIT S 2         
                //split argsg into an array
                // break cmd into array of strings, argsg
                char** fuck = NULL;
                char* f = strtok(cmd, " ");
                argsn = 0; 

                // split string and append tokens to 'argsg' 
                while (f) 
                {
                    fuck = realloc(fuck, (sizeof(char*) * ++argsn));
                    if (fuck == NULL)
                        exit (-1); // memory allocation failed 

                    fuck[argsn-1] = f;
                    f = strtok(NULL, " ");
                }

                // realloc one extra element for the last NULL 
                fuck = realloc(fuck, sizeof(char*) * (argsn+1));
                fuck[argsn] = 0;
                // SPLIT E 2
                // fork and execute the last call 
                handle_redirect(fuck, argsn, fuck[0]);
                ret_value = execvp(fuck[0], fuck);
                exit(ret_value); // first edit*************************
            }
            else
            {
                waitpid(control_pid, &ret_value, 0);
                if (kill_me == 1)
                    exit(0);
                else
                    continue;
            }
        } // END OF PIPE HANDLING LOGIC 


        // break cmd into array of strings, argsg
        char** argsg = NULL;
        char* p = strtok(cmd, " ");
        int argsn = 0; 

        // split string and append tokens to 'argsg' 
        while (p) 
        {
            argsg = realloc(argsg, (sizeof(char*) * ++argsn));
            if (argsg == NULL)
                exit (-1); // memory allocation failed 

            argsg[argsn-1] = p;
            p = strtok(NULL, " ");
        }

        // realloc one extra element for the last NULL 
        argsg = realloc(argsg, sizeof(char*) * (argsn+1));
        argsg[argsn] = 0;

        if (argsg[0] == NULL) // check for no args
            continue;
        else
            cmd = argsg[0];

// ====================================== begin handling builtins here =======================================

        if (is_builtin(cmd, argsn)) // check for a builtin
        {
            exec_builtin(cmd, argsn, argsg);
        }
        else  // The command is not builtin and must be executing
        {
            // search cmd for a /
            int foundslash = 0;
            for (int i=0;i<strlen(cmd);i++)  
            {
                if (cmd[i] == '/')
                {
                    foundslash = 1;
                    break;
                }
            }
            if (foundslash == 1) // If it contains a /
            {
                 //  check that it exists (stat syscall)
                struct stat buff;
                int exist = stat(cmd, &buff); // exist will be < 0 on fail
                if (exist >= 0) //  If so, pass it to exec
                {
                    if (kill_me == 0)
                    {
                        //int status;
                        pid_t child = fork();
                        if (child == 0)
                        {
                            handle_redirect(argsg, argsn, cmd);
                            //execv(cmd, argsg);
                            exit(0);
                        }
                        else 
                            waitpid(child, &ret_value, 0);
                    }
                    else 
                    {
                        handle_redirect(argsg, argsn, cmd);
                        exit(0);
                    }
                }
            }
            else // Else, no /, check the PATH 
            {
                // copy path to temp var to preserve string through the split
                char* temp_path_var = getenv("PATH");
                char pathvar[sizeof(temp_path_var)+1];
                strcpy(pathvar, temp_path_var);

                // parse path
                char** paths = NULL;
                char* c = strtok(pathvar, ":");
                int n_paths = 0;

                // split string and append tokens to 'paths' 
                while (c) 
                {
                    paths = realloc(paths, (sizeof(char*) * ++n_paths));
                    if (paths == NULL)
                        exit (-1); // memory allocation failed 

                    paths[n_paths-1] = c;
                    c = strtok(NULL, ":");
                }
                // realloc one extra element for the last NULL 
                paths = realloc(paths, sizeof(char*) * (n_paths+1));
                paths[n_paths] = 0;

                // for each string in paths
                for (int i=0; i<n_paths; i++) 
                {
                    // concat with cmd
                    char temp_cmd[1024];
                    memset(temp_cmd, '\0', 1024);
                    // build the path you are trying to execute
                    strcat(temp_cmd, paths[i]); 
                    strcat(temp_cmd, "/");
                    strcat(temp_cmd, cmd); 
                    // use stats to test if it exists
                    struct stat buff;
                    int exist = stat(temp_cmd, &buff); // exist will be < 0 on fail
                    if (exist >= 0) // If it is a real file
                    {
                        // fork & execute
                        int status;
                        pid_t child = fork();
                        // check for > and < here ???
                        if (child == 0)
                        {
                            handle_redirect(argsg, argsn, temp_cmd);
                            //execv(temp_cmd, argsg); // pass it args array (char* []){"ls"}
                            exit(0);
                        }
                        else 
                        {
                            waitpid(child, &status, 0);
                            break;
                        }
                    }
                }
            }
        }

        //All your debug print statments should be surrounded by this #ifdef
        //block. Use the debug target in the makefile to run with these enabled.
        #ifdef DEBUG
        fprintf(stderr, "Length of command entered: %ld\n", strlen(cmd));
        #endif
        //You WILL lose points if your shell prints out garbage values.
    }

    //Don't forget to free allocated memory, and close file descriptors.
    free(cmd);
    //WE WILL CHECK VALGRIND!
    if (kill_me == 1)
        exit(0);
    return EXIT_SUCCESS;
}

/*
 * ag - args array
 * an - number of elenents in args array
 * command - command to run
 */
void handle_redirect(char** ag, int an, char* command)
{
    // **Code executed here will only be called after a fork()**
    int bracket = -1;
    
    // search for < or >
    for (int i = 0; i < an; i++)
    {
        if (strcmp(ag[i], ">")==0)
        {
            // redirect output from stdout to args[i+1]
            int out = open(ag[i+1], O_RDWR|O_CREAT, 0600); // overwrite, don't append
            dup2(out, 1);
            close(out);

            // get index of bracket (if first found)
            if (bracket == -1)
                bracket = i;
        }
        else if (strcmp(ag[i], "<")==0)
        {
            // do the same shit for the left bracket
            // redirect input from stdin to args[i+1]

            // Check that the file is real
            struct stat buff;
            int exist = stat(ag[i+1], &buff); // exist will be < 0 on fail
            if (exist < 0) // If it is a real file
            {
                ret_value = 1;
                bracket = -2;
                break;
            }

            int in = open(ag[i+1], O_RDWR, 0600); // overwrite, don't append
            dup2(in, 0);
            close(in);

            //get index of bracket (if first found)
            if (bracket == -1)
                bracket = i;
        }
    }
    if (bracket == -1) // check to see if a bracket was found above
    {
        if (is_builtin(command, an))
            exec_builtin_plus(command);
        else
            execvp(command, ag); // no bracket, pass ag as arguments
        //printf("execv returned\n");
    }
    else if (bracket != -2)
    {
        // bracket found, get args array
        char* parsed_args[bracket+1];
        //memset(parsed_args, '\0', sizeof(bracket));
        //printf("the error is in the loop you cunt\n");
        for (int i=0;i<bracket;i++)
        {
            parsed_args[i] = ag[i]; // copy args[i] to temp[i]
        }
        parsed_args[bracket] = 0;

        if (is_builtin(command, bracket))
        {
            exec_builtin_plus(command);
            //exec_builtin(command, bracket, parsed_args);
        }
        else
            execvp(command, parsed_args); // (char*[]){"ls", "-l", NULL} for testing
    }
}

void handle_cd(char** ag)
{
    if (strcmp(ag[1], "..")==0)
    {
        getcwd(prev_dir, 1023);
        chdir("..");
        ret_value = 0;
    }
    else if (strcmp(ag[1], "-")==0)
    {
        // only run if there is a previous deirectory
        if (strcmp(prev_dir, "NOT SET") != 0)
        {
            char temp[1024]; // create a temp
            strcpy(temp, prev_dir); // temp = prev
            getcwd(prev_dir, 1023); // prev = current
            chdir(temp); // chrdir to temp
        }
    }
    else if (strcmp(ag[1], ".") != 0)
    {
        DIR* dir = opendir(ag[1]);
        if (dir)
        {
            // directory exists
            closedir(dir);
            chdir(ag[1]);
        }
       // ELSE, directory doesn't exist or some error occurred
    }
}

void handle_chclr(char** ag)
{
    // ag[0] = chclr
    // ag[1] = SETTING
    // ag[2] = COLOR
    // ag[3] = BOLD
    if (strcmp(ag[1], "user") == 0)
    {   
        // process color
        if (strcmp(ag[2],"red")==0)
            ucolor = RED;
        else if(strcmp(ag[2], "blue")==0)
            ucolor = BLUE;
        else if(strcmp(ag[2], "green")==0)
            ucolor = GREEN;
        else if(strcmp(ag[2], "yellow")==0)
            ucolor = YELLOW;
        else if(strcmp(ag[2], "cyan")==0)
            ucolor = CYAN;
        else if(strcmp(ag[2], "magenta")==0)
            ucolor = MAGENTA;
        else if(strcmp(ag[2], "black")==0)
            ucolor = BLACK;
        else if(strcmp(ag[2], "white")==0)
            ucolor = WHITE;

        // process bold
        if (strcmp(ag[3], "1")==0)
        {
            ubold = BOLD;
        }
        else if (strcmp(ag[3], "0")==0)
        {
            ubold = NOTBOLD;
        }
    }
    else if (strcmp(ag[1], "machine") == 0)
    {
        // process color
        if (strcmp(ag[2],"red")==0)
            mcolor = RED;
        else if(strcmp(ag[2], "blue")==0)
            mcolor = BLUE;
        else if(strcmp(ag[2], "green")==0)
            mcolor = GREEN;
        else if(strcmp(ag[2], "yellow")==0)
            mcolor = YELLOW;
        else if(strcmp(ag[2], "cyan")==0)
            mcolor = CYAN;
        else if(strcmp(ag[2], "magenta")==0)
            mcolor = MAGENTA;
        else if(strcmp(ag[2], "black")==0)
            mcolor = BLACK;
        else if(strcmp(ag[2], "white")==0)
            mcolor = WHITE;

        // process bold
        if (strcmp(ag[3], "1")==0)
            mbold = BOLD;
        else if (strcmp(ag[3], "0")==0)
            mbold = NOTBOLD;
    }
}

void handle_chpmt(char** ag)
{
    if (strcmp(ag[1], "user")==0)
    {
        if (strcmp(ag[2], "1")==0) // toggle = 1 and uflag == 0
            uflag = 1;
        else if (strcmp(ag[2], "2")==0) // toggle = 2 and uflag == 1
            uflag = 0;
    }
    else if (strcmp(ag[1], "machine")==0)
    {
        if (strcmp(ag[2], "1")==0)
            mflag = 1;
        else if (strcmp(ag[2], "2")==0 && mflag == 1)
            mflag = 0;
    }
}

void rebuild_prompt()
{
    char* username = getenv("USER");
    char machinename[1024];
    gethostname(machinename, 1023);

    printf("%s", WHITE); // keep this print
    if (uflag == 1) // create prompt with user field
    {
        memset(prompt, '\0', 4000);   // reset prompt to a blank string
        strcat(prompt, "sfish-");     // sfish-
        strcat(prompt, ubold);        // set user boldness
        strcat(prompt, ucolor);       // set user color
        strcat(prompt, username);     // user
        strcat(prompt, NOTBOLD);      // reset boldness
        strcat(prompt, WHITE);        // set color to white
        if (mflag == 1)
        {
            strcat(prompt, "@");          // @
            strcat(prompt, mbold);        // set machine boldness
            strcat(prompt, mcolor);       // set machine color
            strcat(prompt, machinename);  // machine
            strcat(prompt, NOTBOLD);      // reset boldness
            strcat(prompt, WHITE);        // set color to white
        }
        strcat(prompt, ":[");        // :[
    
        //char cwd[1024];
        getcwd(cwd, sizeof(cwd));    // getcwd

        if (strcmp(cwd, "/home")==0) // if = home
            strcat(prompt, "~");     // ~
        else                         // else
            strcat(prompt, (cwd+5)); // cwd + 5 (/home)

        strcat(prompt, "]>");        // ]>
    }
    else if (uflag == 0) // create prompt without user field
    {
        memset(prompt, '\0', 4000); // reset prompt to a blank string
        if (mflag==1)
        {
            strcat(prompt, "sfish-");     // sfish-
            strcat(prompt, mbold);        // set machine boldness
            strcat(prompt, mcolor);       // set machine color
            strcat(prompt, machinename);  // machine
            strcat(prompt, NOTBOLD);      // reset boldness
            strcat(prompt, WHITE);        // set color to white
        }
        else 
            strcat(prompt, "sfish");
            
        strcat(prompt, ":[");         // :[
        //char cwd[1024];
        getcwd(cwd, sizeof(cwd)); // getcwd

        if (strcmp(cwd, "/home")==0) // if = home
            strcat(prompt, "~"); // ~
        else // else
            strcat(prompt, (cwd+5)); // cwd + 5 (/home)

        strcat(prompt, "]>");         // ]>
    }
}

void print_help()
{
    printf("Builtin Functions:\n");
    printf("help  - print a list of builtins and their basic usage\n");
    printf("exit  - exits the shell\n");
    printf("cd    - changes the current working directory of the shell\n");
    printf("pwd   - prints the absolute path of the current working directory\n");
    printf("prt   - prints the return code of the command last executed\n");
    printf("chpmt - change prompt settings\n");
    printf("chclr - change prompt colors\n");
    printf("jobs  - prints JID, status, PID, and name of all jobs running in the background \n");
    printf("fg    - fg PID|JID - make job number in list go to foreground\n\n");
    printf("bg    - bg PID|JID - make job number in list continue executing in background\n");
}

int is_builtin(char* cmd, int n)
{
    if (   strcmp(cmd, "exit") == 0 || strcmp(cmd, "help") == 0 
        || strcmp(cmd, "cd")   == 0 || strcmp(cmd, "pwd")  == 0 
        || strcmp(cmd, "prt")  == 0 || (strcmp(cmd, "chpmt") == 0 && n >= 3) 
        || (strcmp(cmd, "chclr") == 0 && n >= 4)
        || strcmp(cmd, "jobs") == 0 || (strcmp(cmd, "fg") == 0 && n >= 2)
        || (strcmp(cmd, "bg")   == 0 && n >= 2)
        || (strcmp(cmd, "kill") == 0 && n >= 2)
        || strcmp(cmd, "disown") == 0
    )
        return 1;
    else
        return 0;
}

void exec_builtin(char* cmd, int n, char** ag)
{
    if (strcmp(cmd, "exit")==0) // exit - don't fork ------------------------
    {
        ret_value = 0;
        exit(0);
    }
    else if (strcmp(cmd, "help")==0) // help --------------------------------
    {
        pid_t child = fork();
        if (child == 0)
        {
            handle_redirect(ag, n, cmd);
            //print_help();
            //ret_value = 1;
            exit(ret_value);
        }
        else 
        {
            waitpid(child, &ret_value, 0);
            //wait(&ret_value);
        }
    }
    else if (strcmp(cmd, "cd")==0) // cd - don't fork -----------------------
    {
        if (n == 1)
        {
            getcwd(prev_dir, 1023);
            chdir(getenv("HOME"));
            chdir("..");
            ret_value = 0;
        }
        else 
            handle_cd(ag);

        rebuild_prompt();
    }
    else if (strcmp(cmd, "pwd")==0) // pwd ----------------------------------
    {
        pid_t child = fork();
        if (child == 0)
        {
            handle_redirect(ag, n, cmd);
            exit(ret_value);
        }
        if (child != 0)
            wait(&ret_value);
    }
    else if (strcmp(cmd, "prt")==0) // prt ----------------------------------
    {
        pid_t child = fork();
        if (child == 0)
        {
            handle_redirect(ag, n, cmd);
            exit(0);
        }
        else // parent code
            wait(&ret_value);
    }
    else if (strcmp(cmd, "chpmt")==0 && n >= 3) // chpmt - don't fork ---
    {
        handle_chpmt(ag);
        rebuild_prompt();
    }
    else if (strcmp(cmd, "chclr")==0 && n >= 4) // chclr - don't fork ---
    {
        handle_chclr(ag);
        rebuild_prompt();
    }
    else if (strcmp(cmd, "jobs") == 0)
    {
       pid_t child = fork();
        if (child == 0)
        {
            handle_redirect(ag, n, cmd);
            //print_help();
            //ret_value = 1;
            exit(ret_value);
        }
        else 
        {
            waitpid(child, &ret_value, 0);
            //wait(&ret_value);
        } 
    }
    else if (strcmp(cmd, "fg") == 0 && n >= 2)
    {
        // copy argsg[1] to a temp string
        char* strid = malloc(strlen(ag[1]));
        strcpy(strid, ag[1]);

        int found = 0;
        // look for a % in the string
        if (strid[0] == '%')
        {
            // handle JID stuff
            strid++; // advance the cursor to after '%'
            int fg_JID;
            sscanf(strid, "%d", &fg_JID); // cast it to an int
            // search the list for a process with that JID
            Proc* cursor = list_head;
            while (cursor != NULL)
            {
                if (cursor->JID == fg_JID && (strcmp(cursor->status, "Running")==0))
                {
                    waitpid(cursor->PID, &ret_value, 0);
                    found = 1;
                    break;
                }
                else if (cursor->JID == fg_JID && (strcmp(cursor->status, "Stopped")==0))
                {
                    strcpy(cursor->status, "Running");
                    kill(cursor->PID, SIGCONT);
                    waitpid(cursor->PID, &ret_value, 0);
                    found = 1;
                    break;
                }
                cursor = cursor->next;
            }
            if (found == 0)
                printf("[%%%d]: No such job\n", fg_JID);
            free((strid-1));
        }
        else
        {
            // handle pid stuff
            int fg_PID;
            sscanf(strid, "%d", &fg_PID); // cast it to an int
            Proc* cursor = list_head;
            while(cursor!=NULL)
            {
                if (cursor->PID == fg_PID && (strcmp(cursor->status, "Running")==0))
                {
                    waitpid(cursor->PID, &ret_value, 0);
                    found = 1;
                    break;
                }
                else if (cursor->PID == fg_PID && (strcmp(cursor->status, "Stopped")==0))
                {
                    strcpy(cursor->status, "Running");
                    kill(cursor->PID, SIGCONT);
                    waitpid(cursor->PID, &ret_value, 0);
                    found = 1;
                    break;
                }
                cursor = cursor->next;
            }
            if (found == 0)
                printf("%ld: No such process\n", (long int)fg_PID);
            free((strid));
        }   
    }
    else if (strcmp(cmd, "bg") == 0 && n >= 2)
    {
        // copy argsg[1] to a temp string
        char* strid = malloc(strlen(ag[1]));
        strcpy(strid, ag[1]);

        int found = 0;
        // look for a % in the string
        if (strid[0] == '%')
        {
            // handle JID stuff
            strid++; // advance the cursor to after '%'
            int fg_JID;
            sscanf(strid, "%d", &fg_JID); // cast it to an int
            // search the list for a process with that JID
            Proc* cursor = list_head;
            while (cursor != NULL)
            {
                if (cursor->JID == fg_JID && (strcmp(cursor->status, "Stopped")==0))
                {
                    strcpy(cursor->status, "Running");
                    kill(cursor->PID, SIGCONT);
                    found = 1;
                    break;
                }
                cursor = cursor->next;
            }
            if (found == 0)
                printf("[%%%d]: No such job\n", fg_JID);
            free((strid-1));
        }
        else
        {
            // handle pid stuff
            int fg_PID;
            sscanf(strid, "%d", &fg_PID); // cast it to an int
            Proc* cursor = list_head;
            while(cursor!=NULL)
            {
                if (cursor->PID == fg_PID && (strcmp(cursor->status, "Stopped")==0))
                {
                    strcpy(cursor->status, "Running");
                    kill(cursor->PID, SIGCONT);
                    found = 1;
                    break;
                }
                cursor = cursor->next;
            }
            if (found == 0)
                printf("%ld: No such process\n", (long int)fg_PID);
            free((strid));
        } 
    }
    else if (strcmp(cmd, "kill")==0 && n >= 2)
    {
        // setup id and signal to use
        char* strid;
        int signal = SIGTERM;
        if (n > 2) // n = 3+, user sent a signal
        {
            char* strsig = malloc(strlen(ag[1]));
            strcpy(strsig, ag[1]);
            sscanf(strsig, "%d", &signal);

            strid = malloc(strlen(ag[2]));
            strcpy(strid, ag[2]);
        }
        else // n = 2, default to SIGTERM
        {
            strid = malloc(strlen(ag[1]));
            strcpy(strid, ag[1]);
        }

        // signal should be between 1 and 31 (inclusive)
        if (signal > 31 || signal < 1)
            return;

        // look for a % in the string
        if (strid[0] == '%') // JID
        {
            // handle JID stuff
            strid++; // advance the cursor to after '%'
            int fg_JID;
            sscanf(strid, "%d", &fg_JID); // cast it to an int

            // search the list for a process with that JID
            Proc* cursor = list_head;
            while (cursor != NULL)
            {
                if (cursor->JID == fg_JID)
                {
                    strcpy(cursor->status, "Stopped");
                    int r = kill(cursor->PID, signal);
                    if (r == 0)
                        printf("[%d] %d stopped by signal %d\n", cursor->JID, cursor->PID, signal);
                    break;
                }
                cursor = cursor->next;
            }
            free((strid-1));
        }
        else // PID
        {
            // handle pid stuff
            int fg_PID;
            sscanf(strid, "%d", &fg_PID); // cast it to an int
            Proc* cursor = list_head;
            while(cursor!=NULL)
            {
                if (cursor->PID == fg_PID)
                {
                    strcpy(cursor->status, "Stopped");
                    int r = kill(cursor->PID, signal);
                    if (r == 0)
                        printf("[%d] %d stopped by signal %d\n", cursor->JID, cursor->PID, signal);
                    break;
                }
                cursor = cursor->next;
            }
            free((strid));
        }   
    }
    else if (strcmp(cmd, "disown")==0)
    {
        if (n == 1)
            reap_list(-2);
        else // the user specified a JID or PID
        {
            // copy argsg[1] to a temp string
            char* strid = malloc(strlen(ag[1]));
            strcpy(strid, ag[1]);

            int found = 0;
            // look for a % in the string
            if (strid[0] == '%')
            {
                // handle JID stuff
                strid++; // advance the cursor to after '%'
                int fg_JID;
                sscanf(strid, "%d", &fg_JID); // cast it to an int
                // search the list for a process with that JID
                Proc* cursor = list_head;
                while (cursor != NULL)
                {
                    if (cursor->JID == fg_JID)
                    {
                        reap_list(cursor->JID);
                        found = 1;
                        break;
                    }
                    cursor = cursor->next;
                }
                if (found == 0)
                    printf("[%%%d]: No such job\n", fg_JID);
                free((strid-1));
            }
            else
            {
                // handle pid stuff
                int fg_PID;
                sscanf(strid, "%d", &fg_PID); // cast it to an int
                Proc* cursor = list_head;
                while(cursor!=NULL)
                {
                    if (cursor->PID == fg_PID)
                    {
                        reap_list(cursor->JID);
                        found = 1;
                        break;
                    }
                    cursor = cursor->next;
                }
                if (found == 0)
                    printf("%ld: No such process\n", (long int)fg_PID);
                free((strid));
            }
        }   
    }
}

void exec_builtin_plus(char* cmd)
{
    if (strcmp(cmd, "help")==0) // help -------------------------------------
    {
        pid_t child = fork();
        if (child == 0)
        {
            print_help();
            ret_value = 1;
            exit(ret_value);
        }
        else 
        {
            waitpid(child, &ret_value, 0);
            //wait(&ret_value);
        }
    }
    else if (strcmp(cmd, "pwd")==0) // pwd ----------------------------------
    {
        pid_t child = fork();
        if (child == 0)
        {
            //char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);
            exit(0);
        }
        if (child != 0)
            wait(&ret_value);
    }
    else if (strcmp(cmd, "prt")==0) // prt ----------------------------------
    {
        if (ret_value != 2121) // does ret == default?
        {
            printf("Previous return code is -> %d\n", ret_value);
            ret_value = 0;
        }
        else 
        {
            printf("Previous return code is -> %d\n", ret_value);
            ret_value = 1;
        }
        exit(0);
    }
    else if (strcmp(cmd, "jobs") == 0) // jobs ------------------------------
    {
       //pid_t child = fork();
       //if (child == 0)
       //{
            Proc* cursor = list_head;
            while (cursor != NULL) // while cursor != NULL
            {
                // print the info for each job
                printf("[%d]\t%s\t%ld\t%s\n", cursor->JID, cursor->status, (long int)cursor->PID, cursor->name); 
                cursor = cursor->next;
            }
            exit(0);
       //}
       //else
       //{
        //    waitpid(child, &ret_value, 0);
       //} 
    }
}

void reap_list(int JID) 
{
    Proc* cursor = list_head;
    if (JID == -2) // -2 >> Clear Whole List
    {
        // terminate every process in the list
        while(cursor != NULL)
        {
            kill(cursor->PID, SIGTERM);
            cursor = cursor->next;
        }
        // set head to NULL
        list_head = NULL;
    }
    else if (JID == -1) // -1 >> Reap Whole List
    {
       // Remove any dead processes from the list
       while (cursor != NULL)
       {
            int r = kill(cursor->PID, 0);
            if (r == -1)
            {
                // remove from list
                if (cursor->prev != NULL)
                    cursor->prev->next = cursor->next;
                if (cursor->next != NULL)
                    cursor->next->prev = cursor->prev;
                if (cursor == list_head)
                    list_head = cursor->next;
            }
            cursor = cursor->next;
       } 
    }
    else // 0+ >> Reap job with corresponding JID
    {
        while(cursor != NULL)
        {
            // if the process is found, kill it
            if (cursor->JID == JID)
            {
                kill(cursor->PID, SIGTERM); // kill 
                // remove from list
                if (cursor->prev != NULL)
                    cursor->prev->next = cursor->next;
                if (cursor->next != NULL)
                    cursor->next->prev = cursor->prev;
                if (cursor == list_head)
                    list_head = cursor->next;
                break;
            }
            cursor = cursor->next;
        }
    }
}