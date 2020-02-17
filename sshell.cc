/* * Part of the solution for Assignment 1 (CS 464/564), * by Stefan Bruda. */
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <resolv.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tokenize.h"
#include "tcp-utils.h"  // for `readline' only


#define MAXBUF 1024/* 
* Global configuration variables. 
*/
const char* path[] = {"/bin","/usr/bin",0}; // path, null terminated (static)
const char* prompt = "sshell> ";            // prompt
const char* config = "shconfig";            // configuration file
int keepalive = 0;
/* 
* Typical reaper. 
*/

void zombie_reaper (int signal) {    
	int ignore;    
	while (waitpid(-1, &ignore, WNOHANG) >= 0)        
/* NOP */;
}

/* 
  * Dummy reaper, set as signal handler in those cases when we want 
  * really to wait for children.  Used by run_it(). 
  * 
  * Note: we do need a function (that does nothing) for all of this, we 
  * cannot just use SIG_IGN since this is not guaranteed to work 
  * according to the POSIX standard on the matter. 
*/

void block_zombie_reaper (int signal) {    
           /* NOP */
}

/* 
 * run_it(c, a, e, p) executes the command c with arguments a and 
 * within environment p just like execve.  In addition, run_it also 
 * awaits for the completion of c and returns a status integer (which 
 * has the same structure as the integer returned by waitpid). If c 
 * cannot be executed, then run_it attempts to prefix c successively 
 * with the values in p (the path, null terminated) and execute the 
 * result.  The function stops at the first match, and so it executes 
 * at most one external command. 
*/

int socket_OK = 0;

void sigpipe_handler()
{
	printf("SIGPIPE caught\n");
	socket_OK = 0;
}
int open_connection(const unsigned short portno , char * hostname[129],  char message[], int kl)
{
    int sockfd;
    struct sockaddr_in dest;
    char buffer[MAXBUF];
    char  test[200];
    int counter=0;
   // printf("%i",portno);
   //   printf("%s",message);

   // strcpy(test,message);
        /*---Open socket for streaming---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

    /*---Initialize server address/port struct---*/
    memset(&dest,0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(portno);
    if ( inet_aton(hostname[1], &dest.sin_addr) == 0 )
    {
        perror(hostname[1]);
        exit(errno);
    }


    // instal sigpipe handle
  //signal(SIGPIPE,sigpipe_handler);

    /*---Connect to server---*/
    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
    {
        perror("Connect ");
        exit(errno);
    }

    // indicate that socket is OK
    socket_OK=1;
    int y = 0;
    printf("%s",message);
     while(y < 4) {
        y++;
        if(!socket_OK)
            {fprintf(stderr,"no socket");}
       /* if(strlen(message[0])>0 &&  message[strlen(message[0])-1] == '\n')
	{
          message[strlen(message)-1] = '\0';
        }*/
        send(sockfd, message, 1024, 0);
        printf("message sent\n");
        /*---Get "Hello?"---*/
        memset(buffer,0, MAXBUF);
        recv(sockfd, buffer, sizeof(buffer), 0);
        printf("[%d]:%s", ++counter,buffer);
        sleep(1);
}
    /*---Clean up---*/
  if (kl == 0) 
  {close(sockfd);}
    return 0;
} 

/*      char* req = "bill is at the station";
      char ans[128];
      char * ans_ptr = ans;
      int ans_to_go = 128, n = 0;
      send(sockfd,req,strlen(req),0);
    printf("receiving");
      while ((n = recv(sockfd,ans_ptr,ans_to_go,0)) > 0) {
        ans_ptr += n;
        ans_to_go -=n ;
        printf("receiving");  
      }
      close(sockfd);
	return 0;  
}*/


int run_it (const char* command, char* const argv[], char* const envp[], const char** path) {    
// we really want to wait for children so we inhibit the normal    
// handling of SIGCHLD    
	signal(SIGCHLD, block_zombie_reaper);    
	int childp = fork();    
	int status = 0;    
	if (childp == 0) { // child does execve
#ifdef DEBUG        
	fprintf(stderr, "%s: attempting to run (%s)", __FILE__, command);        
	for (int i = 1; argv[i] != 0; i++)            
	fprintf(stderr, " (%s)", argv[i]);        
	fprintf(stderr, "\n");
#endif        
	execve(command, argv, envp);     // attempt to execute with no path prefix...        
	for (size_t i = 0; path[i] != 0; i++) { 
// ... then try the path prefixes            
		char* cp = new char [strlen(path[i]) + strlen(command) + 2];            
		sprintf(cp, "%s/%s", path[i], command);
#ifdef DEBUG            
		fprintf(stderr, "%s: attempting to run (%s)", __FILE__, cp);            
		for (int i = 1; argv[i] != 0; i++)                
		fprintf(stderr, " (%s)", argv[i]);            
		fprintf(stderr, "\n");
#endif            
		execve(cp, argv, envp);            
		delete [] cp;        
}        
// If we get here then all execve calls failed and errno is set        
		char* message = new char [strlen(command)+10];        
		sprintf(message, "exec %s", command);        
		perror(message);        
		delete [] message;        
		exit(errno);   // crucial to exit so that the function does not    
                   // return twice! 
   }    
		else { // parent just waits for child completion     
   		waitpid(childp, &status, 0);      
  // we restore the signal handler now that our baby answered   
     		signal(SIGCHLD, zombie_reaper);    
    		return status;   
 }}
/* * Implements the internal command `more'.  In addition to the file * whose content is to be displayed,
 the function also receives the * terminal dimensions. */ 
int main (int argc, char** argv, char** envp) {   
// int  rport = 0;  // terminal dimensions, read from                                 // the config file   
 char command[129];   // buffer for commands  
  command[128] = '\0';   
 char* com_tok[1024];  
// buffer for the tokenized commands   
 size_t num_tok;     
 // number of tokens 
   printf("Simple shell v2.0.\n");   
 // Config: 
   int confd = open(config, O_RDONLY);  
  if (confd < 0) {   
     perror("config"); 
       fprintf(stderr, "config: cannot open the configuration file.\n");    
    fprintf(stderr, "config: will now attempt to create one.\n");  
      confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);   
     if (confd < 0) {       
     // cannot open the file, giving up.        
    perror(config);   
         fprintf(stderr, "config: giving up...\n");   
         return 1;    
    }  
      close(confd);  
      // re-open the file read-only    
    confd = open(config, O_RDONLY);   
     if (confd < 0) {   
         // cannot open the file, we'll probably never reach this    
        // one but who knows...      
      perror(config);   
         fprintf(stderr, "config: giving up...\n");   
         return 1;    
    }   
 }    // read terminal size  
int k =0; 
char* rhost[129];
unsigned int rport;
  while (k<1) {      
  int n = readline(confd, command, 128);    
    if (n == recv_nodata)  
          break;    
    if (n < 0) {  
          sprintf(command, "config: %s", config); 
           perror(command);        
   		 break;          
	}
    num_tok = str_tokenize(command, com_tok, strlen(command)); 
      if (strcmp(com_tok[0], "RPORT") == 0)
         {
             rport = atoi(com_tok[1]);
              k = k + 0.5; // rhost = &com_tok[1] ;
         }
     if (strcmp(com_tok[0], "RHOST") == 0 ){  
           rhost[1]= com_tok[1];
          k = k + 0.5;
     // lines that do not make sense are hereby ignored  
     }
}
   //printf("%s",rhost[1]);
   close(confd);  
   printf("Server for this session is connected to port:%u & host:%s.\n", rport,rhost[1]);    
// install the typical signal handler for zombie cleanup 
   // (we will inhibit it later when we need a different behaviour,  
  // see run_it)   
 signal(SIGCHLD, zombie_reaper);    
// Command loop:  
  while(1) {     
   // Read input:   
     printf("%s",prompt);   
     fflush(stdin);     
   if (fgets(command, 128, stdin) == 0) {    
        // EOF, will quit   
         printf("\nBye\n");    
        return 0;   
     }     
   // fgets includes the newline in the buffer, get rid of it   
     if(strlen(command) > 0 && command[strlen(command) - 1] == '\n')     
       command[strlen(command) - 1] = '\0';    
    // Tokenize input:    
   char ** real_com = com_tok;  // may need to skip the first   
                                 // token, so we use a pointer to       
                             // access the tokenized command     
   num_tok = str_tokenize(command, real_com, strlen(command));   
     real_com[num_tok] = 0;      // null termination for execve      
  int bg = 0,sol = 0;
 if(strcmp(real_com[0],"!")==0){
   // printf("%s",real_com[1]);
   real_com = com_tok + 1;
   sol=1; 
 } else {
   //  printf("%s",real_com[0]);
     open_connection(rport,rhost,real_com[0],keepalive);
    }

if(sol != 0) {
if (strcmp(real_com[0], "&") == 0) { // background command coming
#ifdef DEBUG        
    fprintf(stderr, "%s: background command\n", __FILE__);
#endif          
  bg = 1;            // discard the first token now that we know that it      
      // specifies a background process...     
       real_com = com_tok + 1;  
        }     
   // ASSERT: num_tok > 0   
     // Process input:
     if (strlen(real_com[0]) == 0) // no command, luser just pressed return  
          continue;
  else if (strcmp(real_com[0],"ka") == 0) { 
  keepalive = 1;
}  else if (strcmp(real_com[0],"cl") == 0) {
  keepalive = 0;
}

  else if (strcmp(real_com[0], "exit") == 0) {      
      printf("Bye\n");      
      return 0;    
    }             else { // external command        
    if (bg) {  // background command, we fork a process that        
               // awaits for its completion    
            int bgp = fork();        
        if (bgp == 0) { // child executing the command          
          int r = run_it(real_com[0], real_com, envp, path);    
                printf("& %s done (%d)\n", real_com[0], WEXITSTATUS(r));      
              if (r != 0) {            
              printf("& %s completed with a non-null exit code\n", real_com[0]);     
               }         
           return 0;      
          }        
        else  // parent goes ahead and accepts the next command 
                   continue;   
         }          
  else {  // foreground command, we execute it and wait for completion           
     int r = run_it(real_com[0], real_com, envp, path);
                if (r != 0) {               
     printf("%s completed with a non-null exit code (%d)\n", real_com[0], WEXITSTATUS(r));        
        }   
         }     
   } 
}
  }}
