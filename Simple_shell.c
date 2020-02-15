#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#define BUFFER_SIZE 32
int main(int argc, char** argv)
{
        //load shconfig
        FILE *fp;
        fp = fopen("shconfig","r");
        if(fp == NULL)
        {
         load_config();
        }
        //command loop
        command_loop();
        return 0;
}
//Function reads user input
char* readfromstream(void)
{
        char *stream = NULL;
        size_t length = 0;
        getline(&stream,&length ,stdin);
        if(stream[0]=='\n')
        {stream[0]='D';}
        return stream;
}
//Funtion uses strtok tokenizer to obtain commands and arguments
char ** read_args (char * line)
{
   int pos = 0, bufsize = BUFFER_SIZE;
   char **stream = malloc(bufsize * sizeof(char*));
   const char *s = " \n\t\r\a";
   char* tok;
    for(tok = strtok(line,s); tok != NULL ; tok = strtok(NULL,s))
  {
        stream[pos] = tok;
        if(pos >= bufsize){
        bufsize +=  BUFFER_SIZE;
        stream = realloc(stream, bufsize * sizeof(char*));
        }
        pos++;
      }
  stream[pos] = 0;
  return stream;
}
//Runs external command without &
int run_external(char** args)
{
 pid_t pid,wpid;
 int status;
 char cmd[100];
 pid = fork();
 if(pid == 0 ){
  strcpy(cmd,"/bin/");
  strcat(cmd,args[0]);
  if(execvp(cmd,args) == -1)
   {
      perror("Error");
   }
  }else
        {
         do{  waitpid(pid, &status , 0);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }
 }
//Function runs external commands with &
int run_externals(char** args)
{
 pid_t pid,wpid;
 int status;
 char cmd[100];
 char cmds[100];
 pid = fork();
 if(pid == 0 ){
  strcpy(cmd,"/bin/");
  strcpy(cmds,"/usr/bin");
  strcat(cmd,args[1]);
  if(execvp(cmd,args) == -1 || execvp(cmds,args) == -1)
   {
      perror("Error");
      exit(0);
  }
 }
  return 0;
}
//Function reads the shconfig file and finds the values of VSIZE and HSIZE
int * read_config()
{
  FILE *fp;
  char check[200];
  char* tok,c;
  int vsize,hsize;
  static int values[2];
  fp = fopen("shconfig","r");
  if(fp == NULL)
  {
   printf("file not found");
  }
 while(!feof(fp))
 {
    if(fgets(check,200,fp) != NULL)
        {
                const char *s=" \n\t\r\a";
                int i = 0,j=0;
                for(tok = strtok(check,s);tok != NULL; tok = strtok(NULL,s))
        {
        //printf("%s",tok);
                if(i==1)
                {
        hsize = atoi(tok);
i=0;
         values[0] = hsize;
     }
      if(j==1)
     {
       vsize = atoi(tok);
        j=0;
       values[1] = vsize;
     }
        if (strcmp(tok,"VSIZE")==0)
        { j = 1;}
        if (strcmp(tok,"HSIZE")==0)
        { i = 1;}
    }
    }
 }
  fclose(fp);
  //printf("%i",values[0]);
  return values;
}
//Function implements the command "more"
int readline(char** args)
{
  FILE *fp;
  char c,check,toprint[3100];
  int bufsize=6000,count=1;
  int * value;
  value=read_config();
  //printf("%d",value[0]);
  fp = fopen(args[1],"r");
 if(fp == NULL)
 {
  printf("file not found");
  return 0;
 }
  c =fgetc(fp);
  int i = 1,j=1;
  while(c != EOF)
  {
if(i >= value[0])
    {
      c = '\n';
      printf("%c",c);
      i = 0;
      j++;     // printf("%i",j);
    }
   if(j >= value[1])
    {
     printf("\npress enter for next page:");
     check = getchar();
     j = 0;
     count++;
     printf("\nPage %i",count);
    }
    printf("%c",c);
    c = fgetc(fp);
    i++;
 }
 fclose(fp);
 return 0;
}
//Function compares commands and choses if its an internal or external command
int launch_shell(char** args)
{
   int j = 0;
   char* internalcommands[]={
   "more",
   "exit",
   };
    if(args[0] == 'D'){run_externals(args);return 0;}
    else if(strcmp(args[0],internalcommands[0])==0)
        {
           readline(args);
                return 0;
        }
    else if(strcmp(args[0],"&")==0)
  {
          run_externals(args);
          return 0;
       }
    else if(strcmp(args[0],internalcommands[1])== 0)
                {
                  exit(0);
                  exit(0);
             }else{
                run_external(args);
                return 0;
             }
}
//Function runs the  loop and passes the values to the appropriate functions
int command_loop()
{
        char*line;
        char**args;
        int check_exit = 0;
                while(1)
        {
                printf(">");
                line = readfromstream();
                args = read_args(line);
                launch_shell(args);
        }
}
//Function creates shconfig if it doesn't exist
void load_config()
{
 int file = open("shconfig",O_RDWR| O_CREAT ,S_IRUSR|S_IWUSR);
 int i=0; char cmessage[256] = "VSIZE 40"; char rmessage[256] = "HSIZE 75";
    if(access("shconfig",F_OK)  == -1)
    {}else {
        write(file,cmessage,strlen(cmessage));
        write(file, "\n",1);
        write(file,rmessage, strlen(rmessage));
        write(file, "\n",1);
        close(file);
  }
}

