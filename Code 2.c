/*
 * Part of the solution for Assignment 1 (CS 464/564),
 * by Stefan Bruda.
 */

#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#define BUFFER_SIZE 32
#include "tokenize.h"
#include "tcp-utils.h"  // for `readline' only

/*
 * Global configuration variables.
 */
const char* path[] = {"/bin","/usr/bin",0}; // path, null terminated (static)
const char* prompt = "sshell> ";            // prompt
const char* config = "shconfig";            // configuration file

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
        for (size_t i = 0; path[i] != 0; i++) { // ... then try the path prefixes
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
    }
}

/*
 * Implements the internal command `more'.  In addition to the file
 * whose content is to be displayed, the function also receives the
 * terminal dimensions.
 */
void do_more(const char* filename, const size_t hsize, const size_t vsize) {
    const size_t maxline = hsize + 256;
    char* line = new char [maxline + 1];
    line[maxline] = '\0';

    // Print some header (useful when we more more than one file)
    printf("--- more: %s ---\n", filename);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        sprintf(line, "more: %s", filename);
        perror(line);
        delete [] line;
        return;
    }

    // main loop
    while(1) {
        for (size_t i = 0; i < vsize; i++) {
            int n = readline(fd, line, maxline);
            if (n < 0) {
                if (n != recv_nodata) {  // error condition
                    sprintf(line, "more: %s", filename);
                    perror(line);
                }
                // End of file
                close(fd);
                delete [] line;
                return;
            }
            line[hsize] = '\0';  // trim longer lines
            printf("%s\n", line);
        }
        // next page...
        printf(":");
        fflush(stdout);
        fgets(line, 10, stdin);
        if (line[0] != ' ') {
            close(fd);
            delete [] line;
            return;
        }
    }
    delete [] line;
}


char* RHOST;// 
char* RPORT;
//From client.cc file
int socketconnect(char* message) {
	const int ALEN = 256;
	char req[ALEN];
	char ans[ALEN];



	int sd = connectbyport(RHOST, RPORT);
    if (sd == err_host) {
        fprintf(stderr, "Cannot find host %s.\n", RHOST);
        return 1;
    }
    if (sd < 0) {
        perror("connectbyport");
        return 1;
    }

int y = 0;
    // we now have a valid, connected socket
    printf("Connected to %s on port %s.\n", RHOST, RPORT);
        int n;
	while (1) {
        while ((n = recv_nonblock(sd,ans,ALEN-1,500)) != recv_nodata) {
            if (n == 0) {
                shutdown(sd, SHUT_RDWR);
                close(sd);
                printf("Connection closed by %s.\n", RHOST);
                return 0;
            }
            if (n < 0) {
                perror("recv_nonblock");
                shutdown(sd, SHUT_WR);
                close(sd);
                break;
            }
            ans[n] = '\0';
            printf("%s", ans);
            fflush(stdout);
            y++;
         }
        printf("> ");
        fflush(stdout);
        if( y >= 0)
        {
          y = 1;
         // fgets(req,256,stdin); 
         strcpy(req,message); 
         printf("%s",message);
           if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
             req[strlen(req) - 1] = '\0';
        printf(" --> %s\n", req);
        fflush(stdout);

        send(sd,req,strlen(req),0);
        send(sd,"\n",1,0);
                printf("message sent\n");

        }
        //fgets(req,256,argv[0]);
        // eat up the terminating newline
	
    }

}




int main (int argc, char** argv, char** envp) {
    size_t hsize = 0, vsize = 0;  // terminal dimensions, read from
                                  // the config file
    char command[129];   // buffer for commands
    command[128] = '\0';
    char* com_tok[129];  // buffer for the tokenized commands
    size_t num_tok;      // number of tokens

	RHOST = (char*)malloc(256*sizeof(char));
	RPORT = (char*)malloc(6*sizeof(char));
    printf("Simple shell v1.0.\n");

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
    }

    // read terminal size
    while (hsize == 0 || vsize == 0) {
        int n = readline(confd, command, 128);
        if (n == recv_nodata)
            break;
        if (n < 0) {
            sprintf(command, "config: %s", config);
            perror(command);
            break;
        }
        num_tok = str_tokenize(command, com_tok, strlen(command));
        // note: VSIZE and HSIZE can be present in any order in the
        // configuration file, so we keep reading it until the
        // dimensions are set (or there are no more lines to read)
        if (strcmp(com_tok[0], "VSIZE") == 0 && atol(com_tok[1]) > 0)
            vsize = atol(com_tok[1]);
        else if (strcmp(com_tok[0], "HSIZE") == 0 && atol(com_tok[1]) > 0)
            hsize = atol(com_tok[1]);

		// Because they are converted to strings and not long integers
		else if (strcmp(com_tok[0], "RHOST") == 0 && strlen(com_tok[1]) > 0)
			strcpy(RHOST, "10.18.0.22");
		else if (strcmp(com_tok[0], "RPORT") == 0 && strlen(com_tok[1]) > 0)
			strcpy(RPORT, "9001");
        // lines that do not make sense are hereby ignored
    }
    close(confd);

    if (hsize <= 0) {
        // invalid horizontal size, will use defaults (and write them
        // in the configuration file)
        hsize = 75;
        confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
        write(confd, "HSIZE 75\n", strlen( "HSIZE 75\n"));
        close(confd);
        fprintf(stderr, "%s: cannot obtain a valid horizontal terminal size, will use the default\n", 
                config);
    }
    if (vsize <= 0) {
        // invalid vertical size, will use defaults (and write them in
        // the configuration file)
        vsize = 40;
        confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
        write(confd, "VSIZE 40\n", strlen( "VSIZE 40\n"));
        close(confd);
        fprintf(stderr, "%s: cannot obtain a valid vertical terminal size, will use the default\n",
                config);
    }

    printf("Terminal set to %ux%u.\n", (unsigned int)hsize, (unsigned int)vsize);

	if (strcmp(com_tok[0], "RHOST")) {

		strcpy(RHOST, "10.18.0.22");
		confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		write(confd, "RHOST 10.18.0.22\n", strlen("RHOST 10.18.0.22\n"));
		close(confd);
	} else {
		fprintf(stderr, "%s: cannot obtain a valid remote hostname, will use the default\n", RHOST);
	}
	
	if (strcmp(com_tok[0], "RPORT")) {
		
		strcpy(RPORT, "9001");
		confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		write(confd, "RPORT 9001\n", strlen("RPORT 9001\n"));
		close(confd);
	} else {
		fprintf(stderr, "%s: cannot obtain a valid remote port number, will use the default\n", RPORT);
	}

	printf("Remote Server set to Host name: %s and Port number: %s.\n", RHOST, RPORT);
	

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
        //char** real_com = com_tok;  // may need to skip the first
                                    // token, so we use a pointer to
                                    // access the tokenized command
        //num_tok = str_tokenize(command, real_com, strlen(command));
        char * cache = new char;
        int pos = 0, bufsize = BUFFER_SIZE;
        char **real_com = (char**)malloc(bufsize * sizeof(char*)); 
        const char *s = " \n\t\r\a";
         char* tok;
        strcpy(cache,command);
   for(tok = strtok(cache,s); tok != NULL ; tok = strtok(NULL,s))
  {
	real_com[pos] = tok;
	if(pos >= bufsize){
	bufsize +=  BUFFER_SIZE;
	real_com = (char**)realloc(real_com, bufsize * sizeof(char*));
}
        pos++;
      }
  real_com[pos] = 0;        // null termination for execve
		int bg = 0;
   //printf("%s",real_com[0]);
        if (strcmp(real_com[0], "&") == 0) { // background command coming
#ifdef DEBUG
            fprintf(stderr, "%s: background command\n", __FILE__);
#endif
            bg = 1;
            // discard the first token now that we know that it
            // specifies a background process...
            real_com = com_tok + 2;
            printf("%s", real_com[0]);
        }
int rem =0,sol = 0;
	if (strcmp(real_com[0], "!") == 0){
			//fprintf(stderr, "%s: remote command\n", __FILE__);                  
                        //printf("check one");
                 sol = 1;
		 real_com = real_com + 1;
                 //printf("%s", real_com[0]);

		}else  { 

                     if (bg) {
                                        int bgp = fork();
                                        if (bgp == 0) { // child connecting and executing to remote serve
                                                //const int argc = sizeof(real_com)/sizeof(char*);
                                                socketconnect(command);
                                                return 0;

                                        } else
                                                continue;
                                } else {  // foreground command, we execute it and wait for completion
                        int r = socketconnect(command);
                       // if (r != 0) {
                    //	printf("%s completed with a non-null exit code (%d)\n", real_com[0], WEXITSTATUS(r));
                     //xy
	              //}
                }

  }   

     // ASSERT: num_tok > 0
if(sol) {
        // Process input:
        if (strlen(real_com[0]) == 0) // no command, luser just pressed return
            continue;

        else if (strcmp(real_com[0], "exit") == 0) {
            printf("Bye\n");
            return 0;
        }

        else if (strcmp(real_com[0], "more") == 0) {
            // note: more never goes into background (any prefixing
            // `&' is ignored)
            if (real_com[1] == 0)
                printf("more: too few arguments\n");
            // list all the files given in the command line arguments
            for (size_t i = 1; real_com[i] != 0; i++) 
                do_more(real_com[i], hsize, vsize);
        }

        else { // external command
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
            } else {// foreground command, execute and wait for completion.
				int r = run_it(real_com[0], real_com, envp, path);
                printf("& %s done (%d)\n", real_com[0], WEXITSTATUS(r));
                if (r != 0) {
                	printf("& %s completed with a non-null exit code (%d)\n", real_com[1], WEXITSTATUS(r));
                   
				}
			}

			/*if (rem == 1) { //execute background command but for connecting, sending/receiving commands to/from remote server
			// awaits for completion
				if (bg) {
					int bgp = fork();
					if (bgp == 0) { // child connecting and executing to remote serve
						//const int argc = sizeof(real_com)/sizeof(char*);
						socketconnect(real_com);
						return 0;

					} else 
						continue;
				} else {  // foreground command, we execute it and wait for completion
                	int r = socketconnect(real_com);
                	if (r != 0) {
                    	printf("%s completed with a non-null exit code (%d)\n", real_com[0], WEXITSTATUS(r));
                	}
            	}
        	} */
		}
	}
     }
 }
