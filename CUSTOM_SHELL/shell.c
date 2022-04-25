/*
# Custom-shell-in-c
A custom shell written in c
Done by group 13
Members:-
Agnel Aaron: 2017010
Bharath Kumar: 2017035
Shahid Nawaz Khan: 2017102
Reference: https://stackoverflow.com/
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define PINK "\x1b[35m"
#define RESET "\x1b[0m"
char hostname[1024];
char *user;
char cwd_path[1024];
int pid_to_kill=-1;
static int *status; 
int child_pid[200];
int child_pid_counter=0;
void signal_handler(int signal)
{
    if (pid_to_kill != -1)
    {
        kill(pid_to_kill, SIGINT);
    }
    //signal(SIGINT, ctrl_c);
}
char *take_input()
{
    char *in_string = malloc(255);
    if (in_string == NULL)
    {
        printf("Unable to read command as no space available in heap \n");
        exit(0);
    }
    else
    {
        int i = 0;
        char chary;
        int count = 1;
        long size = 255;
        chary = getchar();
        while (chary != '\n')
        { // replace with actual enter key detection
            if ((int)chary == -1)
            {
                break;
            }
            in_string[i] = chary;
            i++;
            if (i == 255)    // reallocate memory for every character after the buffer gets 255 characters 
            {
                count++;
                size = count * size;
                in_string = realloc(in_string, size);
            }
            chary = getchar();
        }
        in_string[i] = '\0';
    }
    return in_string;
}
void display_path()
{
    printf(RED "USER:%s@" RESET GREEN "HOST:%s@" RESET PINK "PATH:%s$" RESET, user, hostname, cwd_path);
    // to diplay the user, host and path of current woring directory in a colourful and attractive manner
}
int Parse_input(char *input, char *delimiters, char **output)
{

    output[0] = strtok(input, delimiters);

    int i = 0;

    while ((output[i]) != NULL)
    {
        i++;
        (output[i]) = strtok(NULL, delimiters);
    }
    char *ret_1 = getcwd(cwd_path, 1024);
    if (ret_1 == NULL)
    {
        printf("Couldn't get current path \n");
        exit(1);
    }
    return i;
}
void exec_call(char *argument_list[10000], char *command)
{

    int pid = fork();
    pid_to_kill=pid;    // store the pid of the child process which has to be killed on ^C
    if (pid == -1)
    {
        printf("Fork error. Unable to create child process \n");
    }
    else if (pid == 0)
    {

        int err = execvp(command, argument_list);
        if (err == -1)
        {
            printf("Unknown command \n");
            exit(0);
        }
        exit(0);
    }
    else if (pid > 0)
    {
        wait(NULL);
        pid_to_kill=-1;
    }
}
int command_check(char *command)
{
    if (strlen(command) > 2)
    {
        if (command[0] == '1' && command[1] == '>')
        {
            return 1;
        }
        if (!strcmp(command, "2>&1"))
        {
            return 3;
        }
        if (command[0] == '2' && command[1] == '>')
        {
            return 2;
        }
       
    }
    if(!strcmp(command,"exit")){
        *status=0;
        return 4;
    }
    return 0;
}
void free_space(char **arg,int size){
for (int j = 0; j < size; j++)
{
    arg[j] = NULL;
}
}
void run_single_command(char *input)
{
    char *args[10000];
    char *argument_list[10000];
    int args_size = Parse_input(input, " ", args);
    char *input_file;
    char *output_file;
    int flagin = 0;
    int flagout = 0;
    int flagapp = 0;
    int counter = 0;
    int in_num = dup(0);
    int out_num = dup(1);
    int check_flag;
    int err_flag = 0;
    for (int i = 0; i < args_size; i++)
    {
        check_flag = command_check(args[i]); // checks for default commands
        if(check_flag==4){
            return;
        }
        if (!check_flag)
        {
            if (strcmp(args[i], ">") == 0)    // create output file if it doesn't exist
            {
                if (i == args_size - 1)
                {
                    printf("Syntax error for output redirection. Please specify a file to output to \n");
                }
                else
                {
                    flagout = 1;
                    output_file = args[i + 1];
                    i++;
                    if (access(output_file, F_OK) == -1)
                    {
                        FILE *fp = fopen(output_file, "w");
                        fclose(fp);
                    }
                }
            }
            else if (strcmp(args[i], "<") == 0)    // store the input file
            {
                if (i == args_size - 1)
                {
                    printf("Syntax error for input redirection. Please specify a file to input from \n");
                }
                else
                {
                    flagin = 1;
                    input_file = args[i + 1];
                    i++;
                }
            }
            else if (strcmp(args[i], ">>") == 0)    // create output file if it doesn't exist
            {
                if (i == args_size - 1)
                {
                    printf("Syntax error for output append. Please specify a file to append to \n");
                }
                else
                {
                    flagapp = 1;
                    output_file = args[i + 1];
                    i++;
                    if (access(output_file, F_OK) == -1)
                    {
                        FILE *fp = fopen(output_file, "w");
                        fclose(fp);
                    }
                }
            }
            else
            {

                argument_list[counter] = args[i];
                counter++;
            }
        }
        else
        {
            if (check_flag == 1 || check_flag == 2)    // check for 1 -> filename, 2 -> filename, 2>&1
            {
                char *file_name;
                char **args_in = malloc(2 * sizeof(char *));
                for (int j = 0; j < 2; ++j)
                {
                    args_in[j] = (char *)malloc(100);
                }
                int in_size = Parse_input(args[i], ">", args_in);
                file_name=args_in[1];
                flagout = 1;
                output_file = file_name;
                if (access(output_file, F_OK) == -1)
                {
                    FILE *fp = fopen(output_file, "w");
                    fclose(fp);
                }
                if (check_flag == 2)
                {
                    err_flag = 1;
                }
                //free(file_name);
                // free(args_in[0]);
                // free(args_in[1]);
                // free(args_in);
            }
            else
            {
                dup2(1, 2);
            }
        }
    }
    FILE *fin = NULL, *fout = NULL, *fapp = NULL;
    // open all required files
    if (flagin == 1)
    {
        fin = fopen(input_file, "r");
        if (fin == NULL)
        {
            printf("Error in opening file for input. Please enter valid input file \n");
            return;
        }
        else
        {
            dup2(fileno(fin), 0);
        }
    }
    if (flagout == 1)
    {
        fout = fopen(output_file, "w");
        if (fout == NULL)
        {
            printf("Error in opening file for output. Please enter valid output file \n");
            return;
        }
        else
        {
             if(err_flag==1){
                dup2(fileno(fout), 2);
            }
            else{
            dup2(fileno(fout), 1);
            }
        }
    }
    if (flagapp == 1)
    {
        fapp = fopen(output_file, "a");
        if (fapp == NULL)
        {
            printf("Error in opening file for appending. Please enter valid file \n");
            return;
        }
        else
        {
            dup2(fileno(fapp), 1);
        }
    }
    if (strcmp(args[0], "cd"))
    {
        // execute every command
        exec_call(argument_list, argument_list[0]);
        // close all files
        if (fin)
        {
            dup2(in_num, 0);
            fclose(fin);
        }
        if (fout)
        {
            dup2(out_num, 1);
            fclose(fout);
        }
        if (fapp)
        {
            dup2(out_num, 1);
            fclose(fapp);
        }
        free_space(argument_list,counter);
       free_space(args,args_size);
    }
    else{
        if(!argument_list[2]){
        char original_path[1024];
        for(int i=0;i<1024;i++){
            original_path[i]=cwd_path[i];
        }
        //strcpy(original_path,cwd_path);
        if(counter==1){
            strcpy(cwd_path,"/");
        }
        else{
            if(argument_list[1][0]=='/'){
                 strcpy(cwd_path,argument_list[1]);
            }
            else if(!strcmp("..",argument_list[1])){
                for(int i=strlen(cwd_path)-1;i>0;i--){
                    if(cwd_path[i]=='/'){
                        cwd_path[i]='\0';
                        break;
                    }
                    cwd_path[i]='\0';
                }
            }
            else{
                if(strcmp("/",cwd_path)){
                strcat(cwd_path,"/");
                 }
                strcat(cwd_path,argument_list[1]);
            }
        }
    DIR* dir = opendir(cwd_path);
    if (dir)
    {
        chdir(cwd_path);
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        printf("cd: %s: Not a directory",cwd_path);
        for(int i=0;i<1024;i++){
            cwd_path[i]=original_path[i];
        }
        //strcpy(cwd_path,original_path);
    }
    else
    {
        printf("shell error Please command enter again");
        for(int i=0;i<1024;i++){
            cwd_path[i]=original_path[i];
        }
        //strcpy(cwd_path,original_path);
    }
    
    }
    else{
            printf("cd only accepts one argument \n");
    }
    free_space(argument_list,counter);
    free_space(args,args_size);
}
}
void pipe_input(char **input, int size)
{
    int pid;
    int fd[2];
    int ret;
    int fd_read;    // extra file descriptor used to take input from the read end
    for (int i = 0; i < size; i++)
    {
        
        ret = pipe(fd);
        if (ret == -1)
        {
            printf("Unable to create pipe \n");
            return;
        }
        pid = fork();
        if (pid == 0)
        {
            close(0);
            dup(fd_read);    // as f[0] has to be closed during piping in order to enable writing. Hence, we use fd_read descriptor to read
            if (*status == 0) 
            {
               exit(1);
            }
            if (i != (size - 1))
            {
                close(1);
                dup(fd[1]);
            }
            close(fd[0]);
            run_single_command(*(input + i));
            exit(1);
        }
        else if (pid > 0)
        {
            (child_pid[child_pid_counter])=pid;
            child_pid_counter++;
            if (*status == 0)
            {
                return;
            }
            close(fd[1]);
            fd_read = fd[0];
        }
        else
        {

            printf("Unable to fork \n");
        }
    }
}
void start_shell()
{
    *status = 1;
    signal(SIGINT, signal_handler);
    do
    {
        display_path();     // displays the user, hostname, and path
        int pipe_req = 0;
        char *input_comand = malloc(255);
        input_comand = take_input();  // takes input of unknown length until it encounters newline character ('\n')
        if (input_comand[0] == 0)
        {
            continue;
        }
        char *args[10000];
        int args_size = Parse_input(input_comand, "|", args);    // separates input based on location of '|' and stores it in args
        child_pid_counter=0;
        if (args_size > 1)
        {
            pipe_req = 1;
            pipe_input(args, args_size);     // If pipe was present, execute piping
            for(int j=0;j<child_pid_counter;j++){
                    waitpid(child_pid[j], NULL, 0);   // waits for all the children of the main program to exit
            }
        }
        else
        {
          
            run_single_command(*args);    // if no pipe is present, run only one command
        }
        free_space(args,args_size);
        free(input_comand);
    } while (*status);
}
int initialise_var()
{
    user = getenv("USER");
    if (user == NULL)
    {
        printf("Couldn't load user \n");
        exit(1);
    }
    int ret = gethostname(hostname, 1024);
    //hostname gets userid of current system
    if (ret == -1)
    {
        printf("Couldn't load host \n");
        exit(1);
    }
    char *ret_1 = getcwd(cwd_path, 1024);
    //cwd_path gets current path
    if (ret_1 == NULL)
    {
        printf("Couldn't get current path \n");
        exit(1);
    }
}
int main()
{
    status= mmap(NULL, sizeof *status, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //Assigning heap memory for global exit check(between parent and child)
    initialise_var();
    start_shell();
}