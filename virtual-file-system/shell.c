#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <fcntl.h>

#define BUFSIZE 1000
#define INPBUF 100
#define ARGMAX 10
#define GREEN "\x1b[92m"
#define BLUE "\x1b[94m"
#define DEF "\x1B[0m"
#define CYAN "\x1b[96m"

/* implement shell functions
*/
/* Instructions to support :
*  cd <dir>                                 - Done cd
*  pwd                                      - Done pwd
*  mkdir <dir>                              - Done mkdir
*  rmdir <dir>                              - Done rmdir
*  ls (support ls -l)                       - Done ls & -l
*  exit                                     - Done exit
*/

struct _instr {
    char *argval[INPBUF];
    int argcount;
};
typedef struct _instr instruction;

char *input, *input1;
int exitflag = 0;
int filepid, fd[2];
char cwd[BUFSIZE];
char *argval[ARGMAX]; // our local argc, argv
int argcount = 0, inBackground = 0;
int externalIn = 0, externalOut = 0;
char inputfile[INPBUF], outputfile[INPBUF];


void about();

void getInput();

int function_exit();

void function_pwd(char *, int);

void function_cd(char *);

void function_mkdir(char *);

void function_rmdir(char *);

void function_clear();

void nameFile(struct dirent *, char *);

void function_ls();

void function_lsl();

void executable();

void pipe_dup(int, instruction *);

void run_process(int, int, instruction *);

/*Stop processes if running in terminal, close terminal if only Ctrl+C*/
void stopSignal() {
    if (filepid != 0) {
        int temp = filepid;
        kill(filepid, SIGINT);
        filepid = 0;
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopSignal);
    int i;
    int pipe1 = pipe(fd);
    function_clear();
    function_pwd(cwd, 0);

    while (exitflag == 0) {
        externalIn = 0;
        externalOut = 0;
        inBackground = 0;
        printf("%s%s ~> ", DEF, cwd); //print user prompt
        getInput();

        if (strcmp(argval[0], "exit") == 0 || strcmp(argval[0], "z") == 0) {
            function_exit();
        } else if (strcmp(argval[0], "about") == 0 && !inBackground) {
            about();
        } else if (strcmp(argval[0], "pwd") == 0 && !inBackground) {
            function_pwd(cwd, 1);
        } else if (strcmp(argval[0], "cd") == 0 && !inBackground) {
            char *path = argval[1];
            function_cd(path);
        } else if (strcmp(argval[0], "mkdir") == 0 && !inBackground) {
            char *foldername = argval[1];
            function_mkdir(foldername);
        } else if (strcmp(argval[0], "rmdir") == 0 && !inBackground) {
            char *foldername = argval[1];
            function_rmdir(foldername);
        } else if (strcmp(argval[0], "clear") == 0 && !inBackground) {
            function_clear();
        } else if (strcmp(argval[0], "ls") == 0 && !inBackground) {
            char *optional = argval[1];
            if (strcmp(optional, "-l") == 0 && !inBackground) {
                function_lsl();
            } else function_ls();
        } else {
            executable();
        }
    }
}


/*get input containing spaces and tabs and store it in argval*/
void getInput() {
    fflush(stdout); // clear all previous buffers if any
    input = NULL;
    ssize_t buf = 0;
    getline(&input, &buf, stdin);

    // Copy into another string if we need to run special executables
    input1 = (char *) malloc(strlen(input) * sizeof(char));
    strncpy(input1, input, strlen(input));
    argcount = 0;
    inBackground = 0;
    while ((argval[argcount] = strsep(&input, " \t\n")) != NULL && argcount < ARGMAX - 1) {
        if (sizeof(argval[argcount]) == 0) {
        } else argcount++;
        if (strcmp(argval[argcount - 1], "&") == 0) {
            inBackground = 1; //run in inBackground
            return;
        }
    }
    free(input);
}


// Next 2 functions are called by executable() */
/* use execvp to run the command, check path, and handle errors*/
void runprocess(char *cli, char *args[], int count) {
    int ret = execvp(cli, args);
    char *pathm;
    pathm = getenv("PATH");
    char path[1000];
    strcpy(path, pathm);
    strcat(path, ":");
    strcat(path, cwd);
    char *cmd = strtok(path, ":\r\n");
    while (cmd != NULL) {
        char loc_sort[1000];
        strcpy(loc_sort, cmd);
        strcat(loc_sort, "/");
        strcat(loc_sort, cli);
        printf("execvp : %s\n", loc_sort);
        ret = execvp(loc_sort, args);
        if (ret == -1) {
            perror("+--- Error in running executable ");
            exit(0);
        }
        cmd = strtok(NULL, ":\r\n");
    }
}


/* create pipes if required and send proper formatted commands to run_process */
void pipe_dup(int n, instruction *command) {
    int in = 0, fd[2], i;
    int pid, status, pipest;

    if (externalIn) {
        in = open(inputfile, O_RDONLY); // open the file
        if (in < 0) {
            perror("+--- Error in executable : input file ");
        }
    }

    // enters only in case of pipes
    for (i = 1; i < n; i++) {
        pipe(fd);// fd[0] => fd[1] i.e, r=>w
        int id = fork();
        if (id == 0) {

            if (in != 0) {
                dup2(in, 0);
                close(in);
            }
            if (fd[1] != 1) {
                dup2(fd[1], 1);
                close(fd[1]);
            }

            runprocess(command[i - 1].argval[0], command[i - 1].argval, command[i - 1].argcount);
            exit(0);

        } else wait(&pipest);
        close(fd[1]);
        in = fd[0]; // store input for next child, it there is one
    }
    i--; // for final case
    // keep a copy of current file descriptor
    if (in != 0) {
        dup2(in, 0);
    }
    if (externalOut) {
        int ofd = open(outputfile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(ofd, 1);
    }
    runprocess(command[i].argval[0], command[i].argval, command[i].argcount);
}


/* executables like ./a.out */
void executable() {
    instruction command[INPBUF];
    int i = 0, j = 1, status;
    char *curr = strsep(&input1, " \t\n");// need to do all over again
    // since we need to identify distinct commands
    command[0].argval[0] = curr;

    while (curr != NULL) {
        curr = strsep(&input1, " \t\n");
        if (curr == NULL) {
            command[i].argval[j++] = curr;
        } else if (strcmp(curr, "|") == 0) {
            command[i].argval[j++] = NULL;
            command[i].argcount = j;
            j = 0;
            i++;// move to the next instruction
        } else if (strcmp(curr, "<") == 0) {
            externalIn = 1;
            curr = strsep(&input1, " \t\n");
            strcpy(inputfile, curr);
        } else if (strcmp(curr, ">") == 0) {
            externalOut = 1;
            curr = strsep(&input1, " \t\n");
            strcpy(outputfile, curr);
        } else if (strcmp(curr, "&") == 0) {
            inBackground = 1;
        } else {
            command[i].argval[j++] = curr;
        }
    }
    command[i].argval[j++] = NULL; // handle last command separately
    command[i].argcount = j;
    i++;

    // parent process waits for execution and then reads from terminal
    filepid = fork();
    if (filepid == 0) {
        pipe_dup(i, command);
    } else {
        if (inBackground == 0) {
            waitpid(filepid, &status, 0);
        } else {
            printf("+--- Process running in inBackground. PID:%d\n", filepid);
        }
    }
    filepid = 0;
    free(input1);
}

/* Just name printing function*/
void nameFile(struct dirent *name, char *followup) {
    if (name->d_type == DT_REG)          // regular file
    {
        printf("%s%s%s", BLUE, name->d_name, followup);
    } else if (name->d_type == DT_DIR)    // a directory
    {
        printf("%s%s/%s", GREEN, name->d_name, followup);
    } else                              // unknown file types
    {
        printf("%s%s%s", CYAN, name->d_name, followup);
    }
}


/*ls -l  lists date owner file name and so on*/
void function_lsl() {
    int i = 0, total = 0;
    char timer[14];
    struct dirent **listr;
    struct stat details;
    int listn = scandir(".", &listr, 0, alphasort);
    if (listn > 0) {
        printf("%s+--- Total %d objects in this directory ---+\n", CYAN, listn - 2);
        // owner permissions - group permissions - other permissions
        printf("size modified name\n");
        for (i = 0; i < listn; i++) {
            if (strcmp(listr[i]->d_name, ".") == 0 || strcmp(listr[i]->d_name, "..") == 0) {
                continue;
            } else if (stat(listr[i]->d_name, &details) == 0) {
                total += details.st_blocks; // block size
                // file size (bytes) - time modified - name
                printf("%5lld ", (unsigned long long) details.st_size);
                strftime(timer, 14, "%h %d %H:%M", localtime(&details.st_mtime));
                printf("%s ", timer);
                nameFile(listr[i], "\n");
            }
        }
        printf("%s+--- Total %d object contents ---+\n", CYAN, total / 2);
    } else {
        printf("+--- Empty directory ---+\n");
    }
}

/* list cwd contents*/
void function_ls() {
    int i = 0;
    struct dirent **listr;
    int listn = scandir(".", &listr, 0, alphasort);
    if (listn >= 0) {
        printf("%s+--- Total %d objects in this directory ---+\n", CYAN, listn - 2);
        for (i = 0; i < listn; i++) {
            if (strcmp(listr[i]->d_name, ".") == 0 || strcmp(listr[i]->d_name, "..") == 0) {
                continue;
            } else nameFile(listr[i], "    ");
            if (i % 8 == 0) printf("\n");
        }
        printf("\n");
    } else {
        perror("+--- Error in ls ---+");
    }

}


/* clear the screen*/
void function_clear() {
    const char *blank = "\e[1;1H\e[2J";
    write(STDOUT_FILENO, blank, 12);
}

/* remove folder */
void function_rmdir(char *name) {
    int statrm = rmdir(name);
    if (statrm == -1) {
        perror("+--- Error in rmdir ---+");
    }
}


/* Make folder */
void function_mkdir(char *name) {
    int stat = mkdir(name, 0777);// all appropriate permissions
    if (stat == -1) {
        perror("+--- Error in mkdir ---+");
    }
}


/*change directory functionality*/
void function_cd(char *path) {
    int ret = chdir(path);
    if (ret == 0) // path could be changed if cd successful
    {
        function_pwd(cwd, 0);
    } else perror("+--- Error in cd ---+");
}


/*Implement basic exit*/
int function_exit() {
    exitflag = 1;
    return 0; // return 0 to parent process in run.c
}

/* Implement pwd function in shell - 1 prints, 0 stores*/
void function_pwd(char *cwdstr, int command) {
    char temp[BUFSIZE];
    char *path = getcwd(temp, sizeof(temp));
    if (path != NULL) {
        strcpy(cwdstr, temp);
        if (command == 1)  // check if pwd is to be printed
        {
            printf("%s\n", cwdstr);
        }
    } else perror("+--- Error in getcwd() : ---+");

}

void about() {
    char* descr = "Shell for System Programming 2020 Spring class\n ";
    printf("%s", descr);
}