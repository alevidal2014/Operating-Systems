/*
 * Author: Alejandro Vidal  PID: 5913959
 *
 *  Purpose: The purpose of this assignment is to learn to develop multi-process programs in Linux
 *           environment. Learn how to create and terminate processes, suspend and resume processes
 * 
 *  Certification: I hereby certify that this is my own work and none of it is the work of any other person.
 */

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_ARGS 20
#define SIZE 1024

char cmdline[SIZE] = "";
int background[MAX_ARGS];   //Array of all the backgrounds pids
char ps_str[250] = "ps\t";  //Array used for the ps command



// Return the amount of background process running at this time 
int num_back_proc(){
  int i;
  int num = 0;
  for(i = 0; i<MAX_ARGS; i++){                     //Moving through the array of background processes
    if(background[i] != -1)
      num++;
  }
  return num;
}
 
//Function that converts the pids from the array of background processes to a string to process the ps command
void IntArrayToString() {
      int i;
      char str[12]; 
       for(i = 0; i<MAX_ARGS; i++){               //Moving through the array of background processes
          if(background[i] != -1){
            sprintf(str , "%d", background[i]);   //Cast the value from int to str
            strcat(ps_str, str);                  //Concatenate the value to the initial command
            strcat(ps_str, "\t");                 //Add delimiter
                
          }         
       }
       
      strcat(ps_str, "\n");          
   }  

//Looking for new commands. It returns a value for each new command.
int get_commands(char* input){
  int r = 0;
  if(!strcmp(input, "ps"))
    {r=1;}

  if(!strcmp(input, "stop"))
    {r= 2;}

  if(!strcmp(input, "resume"))
    {r=3;}

  if(!strcmp(input, "kill"))
    {r=4;}

  return r;
}

// Initialize the array of background pids
void init_back_array(){
  int i;
  for(i=0; i< MAX_ARGS; i++){      //Moving through the array of background processes
    background[i] = -1;
  }
}

//Clean the backgroung pid array from those that already finished
void clean_backgorund(){
      int i;
      int status;
      int p = -1;
      for(i=0; i< MAX_ARGS; i++){
        if(background[i] != -1){
          if((waitpid(background[i], &status, WNOHANG))>0){  // Look for the process status to see if the process finished 
             background[i] = -1;
          }
        }
      }
}

//Adds a pid to the list of running background processes 
void add_back_pid(int p){
  int i;
  for(i=0; i< MAX_ARGS; i++){
    if(background[i] == -1){
      background[i] = p;
      return; 
    }
  }
  if(i == MAX_ARGS){
    printf("You reach the limit of backgroung processes running at the same time\n");
  }
}

//Function used to separate a string in tokens base on delimiter
int get_args(char* cmdline, char* args[], char* delim){
  
  int i = 0;

  /* if no args */
  if((args[0] = strtok(cmdline, delim)) == NULL) 
    return 0; 

  while((args[++i] = strtok(NULL, delim)) != NULL) {
    if(i >= MAX_ARGS) {
      printf("Too many arguments!\n");
      exit(1);
    }
  }
  /* the last one is always NULL */
  return i;
}


void exit_wait(){
  printf("Waiting for the background processes to finish...\n");   
  int z;
  int numPID = num_back_proc();                                    // number of background process running at this moment
  for(z = 0; z < numPID ; z++) {
    pid_t exitpid = waitpid(background[z], NULL, 0);                // waiting for all the background processes to finish
  }
}

void execute(char* cmdline) 
{
  int back_proc;            //signal to activate the background processes
  int pid;                  //pid of the current process
  
  
  char* comDeli = "\n\t ";  //Delimiter used to separate different commands and arguments
  char* lineDeli = ";";     //Delimiter used to separate multiple commands in the same line

  char* argsLines[MAX_ARGS];  //Array of commands in a single line
  char* args[MAX_ARGS];       //Array of arguments inside a command

  int foreground[MAX_ARGS];   //Array of all the foregrounds pids
  
  
  int p =0;                   // Foreground pids index

  int i;                      //Commands loop control
  
  int exitFlag = 0;           // Flag to control exit the shell
  
  int ncom = get_args(cmdline, argsLines, lineDeli);  //Separating the comandline in multiple commands
  
  //Loop to create different processes 
  for(i = 0; i < ncom; i++) {
   
    clean_backgorund();                                 // Clean background pids array  of processes from those processes that finished already
    int nargs = get_args(argsLines[i], args, comDeli);  // Separating each command and its arguments
    //printf("*********%s\n%s\n"args[0]; args[1]);

    if(nargs <= 0){                                     //Wrong argument list
      continue;
      //exit(0);
    }

    if(!strcmp(args[0], "quit") || !strcmp(args[0], "exit") || !strcmp(args[0], "Ctrl-D")) {  // Exit on 'quit' , 'exit' or 'Ctrl-D'
      
      exitFlag = 1;     //Rise a flag to exit the shell after all the line is executed
      continue;         //Continue to the next command in the same line
    }

    if(!strcmp(args[nargs-1], "&")) {                   // Evaluating the command line for background and foreground process
      back_proc = 1; 
      args[--nargs] = 0; 
    }
    else {
      back_proc = 0;
    }

    int pro = get_commands(args[0]);                    // Evaluating special commands 1-Ps, 2- Stop, 3-Resume, 4-Kill
    
              
        pid = fork();                                   // Creating the process
      
        
        if(pid == 0) { //this is the child 
            
              switch(pro){  // Managing the new commands 0- Other commands 1-Ps, 2- Stop, 3-Resume, 4-Kill
                case 0:
                    execvp(args[0], args);
                    perror("!!!Execution failed!!!\t Incorrect Command:\t");
                    exit(-1);
                    break;
                case 1:   //ps command
                  if(!num_back_proc()){ // No background processes
                    printf("There are no processes running in the background at the momment\n");
                    exit(0);
                  }else{
                    printf("Processes running in the background at the momment:\n");
                    IntArrayToString();                 // Converting the pid array of background processes into a string separated by \t                
                    get_args(ps_str, args, comDeli);    // Tokenazing the string to get the args 
                    execvp(args[0], args);              // Executing the ps command only in background processes
                    perror("!!!Execution failed!!!\t Incorrect Command: \t");
                    exit(-1);  
                  } 
                  break;
                case 2:   // stop command
                  kill(atoi(args[1]), SIGSTOP);
                  exit(0);
                  break;
                case 3:    // resume command
                  kill(atoi(args[1]), SIGCONT);
                  exit(0);
                  break;
                case 4:    // kill command
                  kill(atoi(args[1]), SIGKILL);
                  exit(0);
                  break;                             
              }
        }

        if(pid < 0){ // error occurred 
            perror("fork failed");
            exit(1);
        }

        if(pid > 0) { // parent process 
          if(!back_proc) {
             foreground[p] = pid;           // Add a new foreground process to the array
             p++;
          }else {
            clean_backgorund();             //Clean the background array from defunct processes
            add_back_pid(pid);              // Add a new background process to the array
            printf("PID %d\n", pid);
          }
        }
      

      }
      
      for(i = 0; i < p ; i++) {
          pid_t childpid = waitpid(foreground[i], NULL, 0);  // Wait for foreground process to finish 
      }
     
      if(exitFlag){       //If we have any exit command in this line, exit after waiting for background processes
           exit_wait();   // Wait for bacground process to finish before closing the shell
           exit(0);
      }
 
}

//Reads input from file
int execute_file(char* file_name) {
       
   FILE * f; //File pointer

   // Checking if the File exist 
   if(!(f = fopen(file_name , "r"))) {
      printf("The input File does not exist.\n");
      return 1; //Error code return
   }
  
   //int o = 0;
   // Reading file until the EOF character
   while (!feof(f)) {
      char buffer[1024] = ""; //Line read
      int c = fgetc(f); //current char
      int bufferIndex = 0;  //buffer index
      for (; c != EOF && c != '\n'; c = fgetc(f)) {
         buffer[bufferIndex++] = c;
      }
      
      if(bufferIndex==0 || bufferIndex==1){   //Ignoring Empty line in the file 
        continue;
      } 

      if(c == '\n'){
        buffer[--bufferIndex] = '\n';  
      }   //End of the line
      
      printf("\nExecuting Line:  %s\n", buffer); 
      execute(buffer);   
   }  
   
    exit_wait();  // Wait for bacground process to finish before closing the shell           
   fclose(f);
   return 0;
}

int main (int argc, char* argv [])
{
  
  init_back_array(); //Initializing the background pid array

  // Checking the number of Arguments from the original command line
  if(argc < 0 || argc > 2){
    printf("Incorrect number of Arguments\n");
    printf("Usage: program_name [batch_file_name] (batch_file_name is optional to open batch mode)\n");
    return 0;
  } 

  // Two arguments represents Batch Mode
  if(argc == 2){
    printf("***Batch Mode***\n");
    printf("Reading Lines from batch file...\n");
    execute_file(argv[1]);
    return 0;
  }else{ //No arguments represents Interactive Mode
    
    printf("***Interactive Mode***\n");
    for(;;) {
      printf("Vidal_Shell>> ");
      if(fgets(cmdline, SIZE, stdin) == NULL) {
        perror("fgets failed");
        exit(1);
      }
      execute(cmdline) ;
    }
  }    
  return 0;
}