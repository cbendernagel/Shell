#include <stdlib.h>
#include "320sh.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>


char oldPath[1024];
char* finalCursor = oldPath;
int jobIdCounter = 0;
int returnVal = -1;





/*
  // THIS IS NOW SAFE TO USE, DUPLICATED ARRAY BEFORE USING FUNCTIONS BUILT ON STRTOK
*/
void printAllSysArgs(char* cmdBuffer) {
  //printf("printing buffer | %s\n", cmdBuffer);
  // int rv = 0;
  // rv = rv + 1 -1;
  // char cmdBufferDup[MAX_INPUT];
  // // make a copy of cmdBuffer
  // memset(cmdBufferDup, '\0', 1024);
  // memcpy(cmdBufferDup, cmdBuffer, 1024);

  // char* currentSysArg = getNextSysArg(cmdBufferDup);
  // if (currentSysArg != NULL) {
  //   printf("%s\n", currentSysArg);
  //   // rv = write(1, currentSysArg, strlen(currentSysArg));
  //   while((currentSysArg = getNextSysArg(NULL)) != NULL) {
  //     printf("\n");
  //    // fputs(currentSysArg, stdout);
  //    printf("%s\n", currentSysArg);
  //   }
  // }
  // else {
  //   // do nothing
  // }
  // printf("do we make it here\n");
}

/*
  If we are trying to get the initial first sysarg, pass in a pointer to our character array.
  If we are trying to get any sysarg after the first one, pass in a NULL pointer. 
    The reason for this is that strtok() maintains a static pointer to our previous passed string, so we 
    pass in NULL after the initial one to tell it to find the "next" token (sysarg). 
  * @return Returns pointer to next token, or NULL if no more exist
*/
char* getNextSysArg(char* cmdBuffer) {
  return strtok(cmdBuffer, " ");
}

char* getNextIndividualPath(char* pathBuffer) {
  return strtok(pathBuffer, ":");
}

char* getNextcdArg(char* cdBuffer) {
  return strtok(cdBuffer, "/");
}

char* getNextQuotation(char* cmdBuffer) {
  return strtok(cmdBuffer, "\"");
}
/*
  First we have to search through entire cmdBuffer to determine what the size of our array should be
  @return -1 if something went wrong
*/
int getNumSysArgs(char* cmdBuffer) {
  int numSysArgs = 0;
  char* currentSysArg = getNextSysArg(cmdBuffer);
  if (currentSysArg != NULL) {
    numSysArgs++;
    while((currentSysArg = getNextSysArg(NULL)) != NULL) {
      numSysArgs++;
    }
    return numSysArgs;
  }
  else {
    return -1;
  }
}

/*
  Next we have to determine the length of the longest command 
  @return -1 if something went wrong
*/
int lenLongestSysArg(char* cmdBuffer) {
  int lenLongestSysArg = 0;
  char* currentSysArg = getNextSysArg(cmdBuffer);
  if (currentSysArg != NULL) {
    if (strlen(currentSysArg) > lenLongestSysArg) {
      lenLongestSysArg = strlen(currentSysArg);
    }
    while((currentSysArg = getNextSysArg(NULL)) != NULL) {
      //printf("%d\n", (int)strlen(currentSysArg));
      if (strlen(currentSysArg) > lenLongestSysArg) {
        lenLongestSysArg = strlen(currentSysArg);
      }
    }
    return lenLongestSysArg;
  }
  else {
    return -1;
  }
}

/*
****************************************I NEED TO CHECK FOR CALLOC ERRORS***********************************************
  Reserves memory for a two dimensional array
    sysArgs[numSysArgs][lenLongestSysArg]
*/
char* getAllSysArgs(char* cmdBuffer, int numSysArgs, int lenLongestSysArg) {
    // add 1 to length so each element is null terminated
    lenLongestSysArg++;
    //*********************second increment very important for cd working properly*********************
    lenLongestSysArg++;
    // equivalent of sysArgs[numSysArgs][lenLongestSysArg]
    char* sysArgs = calloc(numSysArgs, (sizeof(char) * lenLongestSysArg));
    int cursor = 0;
    char* currentSysArg = getNextSysArg(cmdBuffer);
    strcpy(sysArgs + cursor, currentSysArg); 
    cursor += lenLongestSysArg;
    while((currentSysArg = getNextSysArg(NULL)) != NULL) {
      strcpy(sysArgs + cursor, currentSysArg); 
      cursor += lenLongestSysArg;
    }
    return sysArgs;
}

/*
****************************************I NEED TO CHECK FOR CALLOC ERRORS***********************************************
  @return NULL if program does not exist in any of the paths included in systemPATH
*/
char* getProgramPath(char* programName) {
  DIR* dirStream;
  struct dirent* dirEntry;
  char* systemPATHOriginal = getenv("PATH");
  // copy this into a new buffer because getNextIndividualPath relies on strtok
  char systemPATH[strlen(systemPATHOriginal)+1];
  memset(systemPATH, '\0', strlen(systemPATHOriginal)+1);
  memcpy(systemPATH, systemPATHOriginal, strlen(systemPATHOriginal)+1);

  //printf("\n\nENV:%s\n\n",systemPATH);
  char* individualPath = getNextIndividualPath(systemPATH);
  if (individualPath != NULL) {
    dirStream = opendir(individualPath); // open directory
    if (dirStream != NULL) {
      //printf("OPEN DIRECTORY: %s\n", individualPath);
      while ((dirEntry = readdir(dirStream)) != NULL) { // search this directory until there's nothing left to search
        //printf("----%s\n", dirEntry->d_name);
        if (strcmp(dirEntry->d_name, programName) == 0) {  // WE HAVE A MATCH
          // concatenate programName to individualPath and return 
          char *programPath = calloc(1, strlen(individualPath) + strlen(programName));
          strcpy(programPath, individualPath);
          strcat(programPath, programName);
          closedir(dirStream);
          return programPath;
        }
      }
      closedir(dirStream);
    }
    while((individualPath = getNextIndividualPath(NULL)) != NULL) { // while loop searching the rest of the individualPaths in systemPath
      dirStream = opendir(individualPath); // open directory
      //printf("OPEN DIRECTORY: %s\n", individualPath);
      if (dirStream != NULL) {
        while ((dirEntry = readdir(dirStream)) != NULL) { // search this directory until there's nothing left to search
          //printf("----%s\n", dirEntry->d_name);
          if (strcmp(dirEntry->d_name, programName) == 0) {
            // concatenate programName to individualPath and return
            char *programPath = calloc(1, strlen(individualPath) + strlen(programName));
            strcpy(programPath, individualPath);
            char slashSpace[1];
            *slashSpace = '/';
            strcat(programPath, slashSpace);
            strcat(programPath, programName);
            closedir(dirStream);
            return programPath;
          }
        }
        closedir(dirStream);
      }
    }
    // If we reach here it means this program does not exist in any of the directories included in systemPATH
    return NULL;
  }
  else {
    return NULL;
  }
}

/*
  @ return -1 if is not a built in command
*/
int isBuiltInCommand(char* command, char** builtInCommands) {
  if(command == NULL)
    return -1;
  for (int i = 0; i < 12; i++) {
    if (strcmp(command, builtInCommands[i]) == 0) {
      return i;
    }
  }
  return -1;
}

int pwdBuiltIn(char* buffer) {
  getcwd(buffer, 1024); // *******************************FIX THIS LATER WHEN WE FIGURE OUT WHERE TO DEFINE MAX_INPUT
  char newLineSpace[1];
  *newLineSpace = '\n';
  strcat(buffer, newLineSpace);
  int v = write(1, buffer, strlen(buffer));
  if (v <= 0) {
    return 0;
  }
  else{
    return 1;
  }
}

void exitBuiltIn() {
  exit(0);
}

int cdBuiltIn(char *const cdArray[]) {
  //_______________________________________________________---
  // cd by itself or cd ~ means home directory
  // cd . means replace with current working directory
  // cd .. means move up a directory
  // cd - means change to the last directory the user was in 
  // ________________________________________________
  char finalPath[1024]; // fix this when we figure out including MAX_INPUT 
  memset(finalPath, '\0', 1024);
  char *cursor = finalPath;
  char charSpace[1];
  DIR* dirStream;

  //printf("\n%s\n", cdArray[0]);
  //printf("\n%s\n", cdArray[1]);

  // struct dirent* dirEntry;
  bool errorFree = true;
  int currentArg = 0;
  while (cdArray[currentArg] != 0) {
    if (currentArg == 0) { // *************************************** FIRST ARGUMENT ***********************************
      if (*cdArray[currentArg] == ' ') { // this means root directory was specified
        // dirStream = opendir(individualPath); // open directory
        *charSpace = '/';
        *cursor = *charSpace;
        cursor++;
      }
      else if (*cdArray[currentArg] == '-') { // change to the last directory the user was in
        if (*oldPath != '\0') {
          memcpy(finalPath, oldPath, 1024);
        }
        else {
          char workingDirectory[1024];
          getcwd(workingDirectory, 1024);
          int len = strlen(workingDirectory);
          strcat(finalPath, workingDirectory);
          cursor = cursor + len;
        }
      }
      else if (*cdArray[currentArg] == '~' || *cdArray[currentArg] == '!') { // replace with home directory
        //printf("\n%s\n", getenv("HOME"));
        char* homePath = getenv("HOME");
        int len = strlen(homePath);
        strcat(finalPath, homePath);
        cursor = cursor + len;
      }
      else if (*cdArray[currentArg] == '.') { // single dot so replace with current working directory
        char workingDirectory[1024];
        getcwd(workingDirectory, 1024);
        int len = strlen(workingDirectory);
        strcat(finalPath, workingDirectory);
        cursor = cursor + len;
        if (*(cdArray[currentArg]+1) == '.') { // double dots so move up a directory
          //printf("\nhere\n");
          char* tempCursor = cursor - 1;
          while(*tempCursor != '/') {
            tempCursor--;
          }
          char* newCursorLocation = tempCursor;
          if (tempCursor == finalPath) { // If we had something like /home or just /, pressing .. should not erase the first slash
            newCursorLocation++; // skip over the slash
            tempCursor++;
            //printf("we make it here\n");
          }
          *charSpace = '\0';
          while(tempCursor < cursor) { // overwrite /currentdirectory with all '\0' characters
            *tempCursor = *charSpace;
            tempCursor++;
          }
          // update Cursor location
          cursor = newCursorLocation;
        }
      }
      else { // we are going down into a directory
          char workingDirectory[1024];
          getcwd(workingDirectory, 1024);
          int len = strlen(workingDirectory);
          strcat(finalPath, workingDirectory);
          cursor = cursor + len;
          *charSpace = '/';
          *cursor = *charSpace;
          cursor++;
          int length = strlen(cdArray[currentArg]);
          strcat(finalPath, cdArray[currentArg]);
          cursor = cursor + length;
      }

    }
    else { // *************************************** NOT FIRST ARGUMENT ***********************************
      if(*cdArray[currentArg] == '.') {
        if (*(cdArray[currentArg]+1) == '.') { // double dots so move up a directory
          char* tempCursor = cursor - 1;
          while(*tempCursor != '/') {
            tempCursor--;
          }
          char* newCursorLocation = tempCursor;
          if (tempCursor == finalPath) { // If we had something like /home or just /, pressing .. should not erase the first slash
            newCursorLocation++; // skip over the slash
            tempCursor++;
            //printf("we make it here\n");
          }
          *charSpace = '\0';
          while(tempCursor < cursor) { // overwrite /currentdirectory with all '\0' characters
            *tempCursor = *charSpace;
            tempCursor++;
          }  
          // update Cursor location
          cursor = newCursorLocation;
        }
        else { // single dot so replace with current working directory
          char workingDirectory[1024];
          getcwd(workingDirectory, 1024);
          int len = strlen(workingDirectory);     
          strcat(finalPath, workingDirectory);
          cursor = cursor + len;
        }
      }
      else { // if not dots must be another directy we're going down into 
        *charSpace = '/';
        *cursor = *charSpace;
        cursor++;
        int length = strlen(cdArray[currentArg]);
        strcat(finalPath, cdArray[currentArg]);
        cursor = cursor + length;
      }
    }
    currentArg++;
  } // end of while loop
  if (errorFree) { // if we simulated all of the cdArgs with no error 
    // now we need to see if the directory represented by finalPath can even be opened
    //dirStream comes into play here
    //printf("final path: %s\n", finalPath);
    dirStream = opendir(finalPath);
    if (dirStream != NULL) {
      char oldWorkingDirectory[1024];
      getcwd(oldWorkingDirectory, 1024);
      // update new working using chdir
      int rv = chdir(finalPath);
      //save old path if was successful
      if (rv == 0) {
        memset(oldPath, '\0', 1024);
        memcpy(oldPath, oldWorkingDirectory, 1024);
        return 0;
      }
      else {
        fprintf(stderr, "%s: error changing directories\n", finalPath);
        return 1;
      }
    }
    else {
      fprintf(stderr, "%s: NOT REACHABLE\n", finalPath);
      return 1;
    }
    
  }
  else { 
    fprintf(stderr, "%s: NOT REACHABLE\n", finalPath);
    return 1;
  }
}


/*
  First we have to search through entire cmdBuffer to determine what the size of our array should be
  @return -1 if something went wrong
*/
int getNumcdArgs(char* cdBuffer) {
  int numcdArgs = 0;
  char* currentcdArg = getNextcdArg(cdBuffer);
  if (currentcdArg != NULL) {
    numcdArgs++;
    while((currentcdArg = getNextcdArg(NULL)) != NULL) {
      numcdArgs++;
    }
    return numcdArgs;
  }
  else {
    return -1;
  }
}

/*
  Next we have to determine the length of the longest cd arg 
  @return -1 if something went wrong
*/
int lenLongestcdArg(char* cdBuffer) {
  int lenLongestcdArg = 0;
  char* currentcdArg = getNextcdArg(cdBuffer);
  if (currentcdArg != NULL) {
    if (strlen(currentcdArg) > lenLongestcdArg) {
      lenLongestcdArg = strlen(currentcdArg);
    }
    while((currentcdArg = getNextcdArg(NULL)) != NULL) {
      //printf("%d\n", (int)strlen(currentSysArg));
      if (strlen(currentcdArg) > lenLongestcdArg) {
        lenLongestcdArg = strlen(currentcdArg);
      }
    }
    return lenLongestcdArg;
  }
  else {
    return -1;
  }
}

/*
****************************************I NEED TO CHECK FOR CALLOC ERRORS***********************************************
  Reserves memory for a two dimensional array
    cdArgs[numcdArgs][lenLongestcdArg]
*/
char* getAllcdArgs(char* cdBuffer, int numcdArgs, int lenLongestcdArg) {
    // add 1 to length so each element is null terminated
    lenLongestcdArg++;
    // equivalent of sysArgs[numSysArgs][lenLongestSysArg]
    char* cdArgs = calloc(numcdArgs, (sizeof(char) * lenLongestcdArg));
    int cursor = 0;
    char* currentcdArg = getNextcdArg(cdBuffer);
    strcpy(cdArgs + cursor, currentcdArg); 
    cursor += lenLongestcdArg;
    while((currentcdArg = getNextcdArg(NULL)) != NULL) {
      strcpy(cdArgs + cursor, currentcdArg); 
      cursor += lenLongestcdArg;
    }
    return cdArgs;
}

/*
  // We pass in original command buffer which will feature replacing of spaces with NAKS Inside of matching
  // "", 
  // We will also replace " with ' ' after we have completed the NAK space replacements
  // EXAMPLE______________________________________________________
  // .mstat "This is an argument" thisIsAnotherOne
  // Modified cmdBuffer
  // -------> ./mstat "ThisNAKisNAKanNAKargument" thisIsAnotherOne
  // -------> .mstat' ' 'ThisNAKisNAKanNAKargument' ' 'thisIsAnotherOne
  //
  // After sysArg[] is created it will look like this
  //          sysArg[0] = ./mstat   ||    sysArg[1] = ThisNAKisNAKanNAKargument   ||    sysArg[2] = thisIsAnotherOne
  // Then we will go through sysArg[] and replace NAK's with spaces
  //          sysArg[0] = ./mstat   ||    sysArg[1] = This is an argument   ||    sysArg[2] = thisIsAnotherOne
  // The purpose of this method is to provide NAK space replacement and ' ' quotation replacement to make the above
  // idea possible. 
*/
void NAKQuoteSupport(char* cmdBuffer) {
  // Everytime we find two quotations 
  // STEP 1: we will replace any spaces in between them with NAK
  // STEP 2: we will replace the " with ' '
  char* cursor = cmdBuffer;
  char* openQuotation = NULL;
  char* closeQuotation = NULL;
  int length = strlen(cmdBuffer);
  char* replaceLocation = NULL;
  char replaceSpace[1];     // a one character array to hold the character we want to replace with
  char* lastCharPosition = cmdBuffer + length -1;    // will us to not search past the end of the string
  while ((openQuotation = strchr(cursor, '\"')) != NULL){
    // found open quotation, now lets try to find our closing one
    cursor = openQuotation + 1; 
    if(cursor <= lastCharPosition){
      if((closeQuotation = strchr(cursor, '\"')) != NULL) {
        // we have found our close quotation
        // now we'll start at openQuotation and start replacing the spaces in between
        cursor = openQuotation;
        while ((replaceLocation = strchr(cursor, ' ')) != NULL && replaceLocation < closeQuotation) {
          *replaceSpace = 21;   // 21 is DEC value of NAK
          *replaceLocation = *replaceSpace;
          cursor = replaceLocation + 1;
        }
        // after we have finished replacing the spaces with NAK its time to replace the open quotation and close quotation with ' '
        *replaceSpace = ' ';
        *openQuotation = *replaceSpace;
        *closeQuotation = *replaceSpace;
        // after we have replaced the spaces with NAKS and the " with ' ' with respect to this set of
        // open and close quotations we  should 
        // update cursor so that we can start looking for the next open quotation in our while loop
        // lets make sure this search starts at the character following the close quotation we just found
        if ((cursor = closeQuotation + 1) < lastCharPosition) {
          // we will search for a new open quotation in the while loop
        }
        else {
          // at end of the string, we can't perform another search for open quotation. lets return
          return;
        }
      }
    } 
    else {
      // we are at the end of the string so we can't search for close quotation, lets return
      return;
    }
  }// end of while that looks for open quotations
}

void shiftBufferUpdateSTDOUT(char* cmdBuffer, char* cursor, char* destinationStart,  char lastCharacter, char* lastCharPosition) {
  // memcpy(cmdBufferDup, cmdBuffer, 1024);
  int numBytesToCopy = lastCharPosition - cursor + 1;
  char duplicateSegment[1024];
  memset(duplicateSegment, '\0', 1024);
  memcpy(duplicateSegment, cursor, 1024);
  // at this point duplicateSegment should be a copy of buffer from cursor ----> end
  memcpy(destinationStart, duplicateSegment, numBytesToCopy);


  if (destinationStart == cursor + 1){ // text replacement in middle of buffer
    // update stdout
    write(1, &lastCharacter, 1);
    write(1, duplicateSegment, numBytesToCopy);
    
    // return stdout cursor
    char* currentCursorPosition = cursor + numBytesToCopy;
    while (currentCursorPosition > cursor) {
      write(1, "\b", 1);
      currentCursorPosition--;
    }
  }
  else { // backspace in middle of buffer
    // update stdout
    write(1, "\b", 1);

    write(1, duplicateSegment, numBytesToCopy);
    write(1, destinationStart + numBytesToCopy, 1);
   //  //do an stdout backspace on original last character
    write(1, "\b \b", 3);

   // move cursor back to new position of original cursor - 1
    for (int i = 0; i < numBytesToCopy; i++) {
      write(1, "\b", 1);
    }
  }
}




job* foregroundListHead = NULL;
job* backgroundListHead = NULL;
job* cursor = NULL;
// shell_process* processHead;


// shell_process* createProcess(char **argv, int inFile, int outFile) {
// 	shell_process* newProcess = malloc(sizeof(struct shell_process));
// 	// PID: 
// 	//		pid will be set by executeProcess function, where we will getpid() of the process that called the executeProcess() function
// 	newProcess -> inFile = inFile;
//   newProcess -> outFile = outFile;
//   newProcess -> finished = false;
// 	newProcess -> suspended = true;
//   newProcess -> next_process = NULL;
// 	// next_process
// 	//		these will be set after successive addProcessToList() calls
// 	return newProcess;
// }

/*
	We call this method after we have 1 process to create the chain of processes needed for a job
*/
// void addProcessToList(shell_process *originProcess, shell_process *newProcess) {
// 	shell_process *cursor = originProcess;
// 	while (cursor != NULL) {
// 		cursor = cursor -> next_process;
// 	}
// 	// cursor should be tail of list
// 	cursor -> next_process = newProcess;
// }


// before creating a job we should:
// have our list of processes assembled and 
// a pgid determined 
job* createJob(char* argv, char* cmdLineBuffer, shell_process *firstProcess, int inFile, int outFile) {
	job* newJob = malloc(sizeof(struct job));
	// PGID:
	// 		pgid will be set by executeJob function, our parent shell after calling fork() will have the pid of a child
	// 		we will set this as the pgid of the job
  newJob -> argv = argv;
	newJob -> enteredCommandLine = cmdLineBuffer;
	newJob -> process = firstProcess;
	// NEXTJOB
	//		nextJob setting is handled in addJobToList() method
  newJob -> nextJob = NULL;

  newJob -> inFile = inFile;
  newJob -> outFile = outFile;
  // PREVJOB
  //    prevJob setting is handled later on
  newJob -> prevJob = NULL;

	return newJob;
}

/*
	Lets add a job to the defined list
*/
void addJobToList( job *newJob) {
	if (jobListHead != NULL) {
    //printf("did we make it? 2.0\n");
		cursor = jobListHead;
		while (cursor -> nextJob != NULL) {
			cursor = cursor -> nextJob;
		}
		// cursor should be tail of list
		cursor -> nextJob = newJob;
    newJob -> prevJob = cursor;
	}
	else { // this list is empty, this job will be the first element
    //printf("did we make it?\t %s\n", );
		jobListHead = newJob;
	}
}

void unblockChildSignals() {
  signal(SIGINT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);
}

void blockParentSignals() {
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);
}

void executeJob(job *jobToBeExecuted, bool foreground, bool debug) {
  pid_t pid;
  int pipes[2];
  int inFile;  
  int outFile;
  returnVal = -1;
  shell_process* currentProcess = jobToBeExecuted -> process;
  while (currentProcess != NULL) {
    if (currentProcess -> next_process != NULL){  // there is another process in the list to pipe to
      pipe(pipes);    // CONNECT THE PROCESSES
      if (currentProcess == processHead) { // special case where we set inFile to be job -> inFile instead of pipes[0]
        inFile = jobToBeExecuted -> inFile;
      }
      outFile = pipes[1];
    }
    else { // last process in job
      if (currentProcess == processHead) {  // special case of a single process in job's process list
        inFile = jobToBeExecuted -> inFile;
      }
      outFile = jobToBeExecuted -> outFile;
    }
    pid = fork();
    if (pid < 0 ) {
      // some weird pid error
      exit(1);
    }
    else if (pid != 0) { 
      ////////////////////////////////////////////
      //            PARENT PROCESS              //
      ////////////////////////////////////////////
      // check to see if pgid has not been set yet, if not lets have parent set job's pgid
      if (jobToBeExecuted -> pgid == 0) { 
        jobToBeExecuted -> pgid = pid;
      }
        currentProcess -> pid = pid;
      //printf("childProcessPID in parent: %d\n", pid);
      // associate this childs' pid with the job pgid
      setpgid(pid, jobToBeExecuted -> pgid);
    }
    else { 
      ////////////////////////////////////////////
      //            CHILD PROCESS               //
      ////////////////////////////////////////////
      pid_t pgidOfJobGroup = jobToBeExecuted -> pgid;
      pid_t childProcessPID = getpid(); 
      //printf("childProcessPID in child: %d\n", childProcessPID);
      if (pgidOfJobGroup == 0) {  //check to see if pgid has not been set yet, if not lets have parent set job's pgid
        jobToBeExecuted -> pgid = childProcessPID;
      }
      // associate this childs' pid with the job pgid
      setpgid(childProcessPID, jobToBeExecuted -> pgid); 
      // set the pid in our process struct
      currentProcess -> pid = childProcessPID;
      // unblock the signals (child has duplicated of parent's (shell's) blocked bits) 
      unblockChildSignals();
      // dup2 to set the correct fds
      if(inFile != STDIN_FILENO){
        currentProcess -> inFile = inFile;
        dup2(inFile, STDIN_FILENO);
        close(inFile);
      }
      if(outFile != STDOUT_FILENO){
        currentProcess -> outFile = outFile;
        dup2(outFile, STDOUT_FILENO);
        close(outFile);
      }
      returnVal = execProcess(currentProcess, debug, jobToBeExecuted -> enteredCommandLine);
    }
    // the parent process will make it down here, must close all intermediate files each time
    // make sure we don't close JOB's inFile and outFile;
    if (inFile != jobToBeExecuted -> inFile) {
      close(inFile);
    }
    if (outFile != jobToBeExecuted -> outFile) {
      close(outFile);
    }
    inFile = pipes[0];
    // update previousPipesOutfile before overwriting it
    currentProcess = currentProcess -> next_process;
  } // end of while
}

/*
  
*/ 
void executeNewJobInForeground(job *jobToBeExecuted, bool debug) {
  jobToBeExecuted -> state = 2;
  jobToBeExecuted -> jobid = ++jobIdCounter;
  foregroundJob = jobToBeExecuted;
  executeJob(jobToBeExecuted, true, debug);
  addJobToList(jobToBeExecuted);
  //printf("job list: %d\n",jobListHead -> pgid);
  waitForForegroundJobToCease(jobToBeExecuted);
}



void continueJobInForeground(job *jobToBeContinued) {
  // have make the process group the foreground process group of our shell here as we 
  // are no longer calling executeJob() when continuing
    markAllProcessesRunning(jobToBeContinued); // this sets fields to false
    jobToBeContinued -> state = 2;
    foregroundJob = jobToBeContinued;
    kill (jobToBeContinued -> pgid, SIGCONT);
    waitForForegroundJobToCease(jobToBeContinued);
}

void executeNewJobInBackground(job *jobToBeExecuted, bool debug) {
  jobToBeExecuted -> state = 2;
  jobToBeExecuted -> jobid = ++jobIdCounter;
  executeJob(jobToBeExecuted, false, debug);
  addJobToList(jobToBeExecuted);
}

void continueJobInBackground(job *jobToBeContinued) {
    markAllProcessesRunning(jobToBeContinued); // this sets fields to false
    jobToBeContinued -> state = 2;
    kill(jobToBeContinued -> pgid, SIGCONT);
}

/*
  Job can cease by either stopping or finishing
*/

void requestJobMoveToForeground(job *jobToMove) {
    kill(jobToMove -> pgid, SIGTSTP);
    continueJobInForeground(jobToMove);
}



void waitForForegroundJobToCease(job *runningJob) {
  pid_t pid = -1;
  int status;
  bool finished = false;
  bool stopped = false;
  bool terminated = false;
  bool stillRunning = true;
  bool ctrl_c = false;
  bool ctrl_z = false;

  char character[1];

  fcntl(processHead -> inFile, F_SETFL, O_NONBLOCK);

  pid = -1;
  while (((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) >= 0) && (errno != ECHILD) && stillRunning) {

    //sleep(2);
    //printf("pid %d\n",pid);

     if (pid > 0){
        //printf("PID: %d\n", pid);

           // we must find the process struct associated with this pid and notate that it is either stopped or finished
        currentProcess = runningJob -> process;
        while(currentProcess != NULL) {
          if (currentProcess -> pid == pid) {
            //printf("CURRENT PROCESS STRUCT PID: %d\n", currentProcess -> pid);
            // has the process stopped or terminated
            if (WIFSTOPPED(status)){  // STOPPED
              //printf("process has stopped\n");
              currentProcess -> stopped = true;
            }
            else if(WIFEXITED(status)){
                //printf("process has finished normally\n");
              currentProcess -> finished = true;
            }
            else if (WIFSIGNALED(status) && ((WTERMSIG(status) == SIGTERM) || WTERMSIG(status) == SIGKILL)) {
                //printf("process has been terminated\n");
                currentProcess -> terminated = true;
            }
            else {
              // do nothing
            }
            // after we have updated the process struct and the statusArray, lets see if this job has fully stopped or fully finished
            stopped = true;
            finished = true;
            terminated = true;
            currentProcess = runningJob -> process;
            while (currentProcess != NULL) {
              if (currentProcess -> stopped == false) {
                stopped = false;
              }
              if (currentProcess -> finished == false) {
                finished = false;
              }
              if (currentProcess -> terminated == false) {
                terminated = false;
              }
              currentProcess = currentProcess -> next_process;
            }
            // set currentProcess to NULL so we stop searching since we found the process struct associated with pid
            currentProcess = NULL;
          }
          else {
            // keep searching for the right process struct
            currentProcess = currentProcess -> next_process;
          }
        } // end of process struct search
    }
    else {
        // TAKE IN POTENTIAL USER INPUT
        if(stopped == false && terminated == false && finished == false){
          if(read(processHead->inFile, character, 1) != 0){
            //lseek(processHead->inFile, -1, SEEK_CUR);
            //write(processHead->inFile, '\0', 1);
            if(*character == 3){               //ctrl-c
              ctrl_c = true;
              goto Loop_break;
            }
            else if(*character == 26){         //ctrl-z
              ctrl_z = true;
              goto Loop_break;
            }
          }
        }
        else {
            //
        }

        // WRITE HEREERHKJFDGKJHFGKJFHGKJFHGKFJHGFGJH
    }

    // // loop through all processes in job, if all processes are stopped or completed, set stillRunning = FALSE
    // bool jobHasCompleted = true;
    // currentProcess = runningJob -> process;
    // while (currentProcess != NULL) {
    //     if ((currentProcess -> stopped == false) && (currentProcess -> finished == false) && (currentProcess -> terminated  == false)) {
    //         jobHasCompleted = false;
    //     }
    //     currentProcess = currentProcess -> next_process;
    // }
    // if (jobHasCompleted) {
    //     stillRunning = false;
    // }
    // else {
    //     // jobHasNotCompleted so leave stillRunning as true
    // }

    //printf("SR: %d | f:%d | t:%d | s:%d\n",stillRunning,finished,terminated,stopped);
    if(finished || terminated || stopped) {
        stillRunning = false;
    }
    pid = -1; 
    //printf("still here\n")
  } // end of waiting for processes to stop/complete
  // RESET ERRNO
  errno = 512;
Loop_break:

  if(ctrl_c == true){
    //do some shit
    killJob(runningJob, 1);
  }
  else if(ctrl_z == true){
    //do some other shit
    suspendJob(runningJob);
  }


  //printf("WE MAKE IT OUT OF WAITPID LOOP\n");
  //printf("pid %d\n",pid);
  if (stopped) {
    //printf("stopped!\n");
    runningJob -> state = 1;
    //printf("stopped!");
  }
  else if (finished) {
    ////////////////////////////////////////////////////////////////////////////////////////////
    ///////// UPDATING LASTSTOPPED JOB AND FOREGROUND JOOB IF RUNNING JOB EQUALS EITHER OF THEM 
    if (lastStoppedJob == runningJob) {
        lastStoppedJob = NULL;
    }
    if (foregroundJob == runningJob) {
        foregroundJob = NULL;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////
    ///////// UPDATING STATE OF JOB  
    if (stopped) {
        runningJob -> state = 1;
        lastStoppedJob = runningJob;
    }
    else if (finished) {
        runningJob -> state = 0;
    }
    else if (terminated) {
        runningJob -> state = -1;
    }
    else {
        runningJob -> state = 2; 
    } 
    //printf("finished!\n");
      if(returnVal == -1){
        returnVal = 0;
      }
      setReturnVar(returnVal);
      if(debug == true){
        fprintf(stderr, "ENDED: ret=%d\n", returnVal);
      }
  } 
  else if (terminated) {
    runningJob -> state = -1;
    //printf("terminated!\n");
  }
}


// 0 == SIGHUP
// 1 == SIGINT
// 2 == SIGTERM 
// 3 == SIGKILL 
void killJob(job* jobToKill, int status) {

  //printf("we enter the killJob method\n");
  if (status == 0) {
    kill(jobToKill -> pgid, SIGHUP);
  }
  else if (status == 1) {
    kill(jobToKill -> pgid, SIGINT);
  }
  else if (status == 2) {
    kill(jobToKill -> pgid, SIGTERM);
  }
  else {
    kill(jobToKill -> pgid, SIGKILL);
  }
  shell_process* currentProcess = jobToKill -> process;
  while (currentProcess != NULL) {
    currentProcess -> terminated = true;
    currentProcess -> stopped = false;
    currentProcess -> finished = false;
    currentProcess = currentProcess -> next_process;
  }
  jobToKill -> state = -1; 
  if (lastStoppedJob == jobToKill) {
    lastStoppedJob = NULL;
  }
  if (foregroundJob == jobToKill) {
    foregroundJob = NULL;
  }
  //printf("we exit the killjob method\n");
}

void suspendJob(job* jobToSuspend) {
  kill(jobToSuspend -> pgid, SIGTSTP);
  jobToSuspend -> state = 1;
  shell_process* currentProcess = jobToSuspend -> process;
  while (currentProcess != NULL) {
    currentProcess -> stopped = true;
    currentProcess = currentProcess -> next_process;
  }
  lastStoppedJob = jobToSuspend;
}


void removeJob(job* jobToRemove) {
  if (jobListHead == jobToRemove) {  // we are removing the head
    if (jobListHead -> nextJob != NULL) {
      job* secondJobInList = jobListHead -> nextJob;
      secondJobInList -> prevJob = NULL;
      free(jobListHead -> argv);
      free(jobListHead -> enteredCommandLine);
      free(jobListHead);
      jobListHead = secondJobInList;
    }
    else { // head was the only thing in the list
      free(jobListHead -> argv);
      free(jobListHead -> enteredCommandLine);
      free(jobListHead);
      // reset jobIdCounter
      jobIdCounter = 0;
      jobListHead = NULL;
    }
  }
  else { // we are not removing the head so we have to actually search for the job we're removing
    job* currentJob = jobListHead;
    job* prevJob = NULL;
    while(currentJob -> nextJob != jobToRemove) {
      currentJob = currentJob -> nextJob;
    }
    // after we have found the job right before the jobToRemove to list
    // we either have to set currentJob -> nextJob = NULL; or currentJob -> nextJob = jobToRemove -> nextJob;
    prevJob = currentJob;
    if (jobToRemove -> nextJob != NULL) {
      currentJob -> nextJob = jobToRemove -> nextJob;
      (jobToRemove -> nextJob) -> prevJob = prevJob;
    }
    else {
      currentJob -> nextJob = NULL;
    }
    free(jobToRemove -> argv);
    free(jobToRemove -> enteredCommandLine);
    free(jobToRemove);
  }
}


void printJobs() {
    //printf("do we make it to top of printjobs method\n");
    updateProcessesAndJobs();
    //printf("do we make it to after update processes in printjobs method\n");

    if (jobListHead == NULL) {
        fprintf(stderr,"There are no jobs to print\n");
    }
    else { // else we have stuff to print
        job* currentJob = jobListHead;
        while(currentJob != NULL) {
            //printf("im here\n");
            char stateBuffer[12];
            int state = currentJob -> state;
            switch(state) {
                case -1: {
                    strcpy(stateBuffer, "Terminated");
                    break;
                }
                case 0:{
                    strcpy(stateBuffer, "Finished");
                    break;
                }
                case 1: {
                    strcpy(stateBuffer, "Stopped");
                    break;
                }
                case 2: {
                    strcpy(stateBuffer, "Running");
                    break;
                }
            }
            printf("[%d] (%d)  | %s |   %s\n", currentJob -> jobid, currentJob -> pgid, stateBuffer, currentJob -> enteredCommandLine);
            currentJob = currentJob -> nextJob;
        }
        // after printing we must remove the jobs who had displayed done or terminated
        job* nextJob = jobListHead;
        while (nextJob != NULL) {
            currentJob = nextJob;
            nextJob = currentJob -> nextJob;
            if (currentJob -> state == -1 || currentJob -> state == 0) {
                removeJob(currentJob);
            }
        }
    }
}


int handleJobsRequest(char** argv) {
    //printf("\njobs request\n");
    if (strcmp(argv[0], "fg") == 0) {
        if (argv[1] == 0) { // user entered just fg, we want to run lastStoppedJob in the foreground 
            if (lastStoppedJob != NULL) {
                suspendJob(lastStoppedJob);
                continueJobInForeground(lastStoppedJob);
                return 0;
            }
            else { // we have to try to find the last suspended job starting at the end of the list
                job* nextLastStoppedJob = getLastStoppedStartingAtListEnd();
                if (nextLastStoppedJob != NULL) {
                    //printf("found nextLastStoppedJob\n");
                    continueJobInBackground(nextLastStoppedJob);
                    return 0;
                }
                else {
                    fprintf(stderr, "no stopped jobs\n");
                    return 1;
                }
            }
        }
        else {  // user entered a second argument and not just fg
            int jobId = getJobIdFromArg(argv, 1);
            if (jobId == -1) { // not valid
                fprintf(stderr, "invalid job id\n");
                return 1;
            }
            else { // lets find job associated with this jobid and run in foreground
                if (jobListHead != NULL) {
                    job* job = getJobByJobId(jobId);
                    bool executionPossible = canJobBeExecuted(job);
                    if (executionPossible) {
                        // lets execute it
                        suspendJob(job);
                        continueJobInForeground(job);
                        return 0;
                    }
                    else { // we can't execute
                        fprintf(stderr, "This job has either finished or terminated\n");
                        return 1;
                    }

                }
                else {
                    fprintf(stderr, "no running jobs\n");
                    return 1;
                }
            }
        }
    }
    else if (strcmp(argv[0], "bg") == 0) {
        if (argv[1] == 0) { // user entered just fg, we want to run lastStoppedJob in the background
            if (lastStoppedJob != NULL) {
                    suspendJob(lastStoppedJob);
                    continueJobInBackground(lastStoppedJob);
                    return 0;
            }
            else { // we have to try to find the last suspended job starting at the end of the list
                if (jobListHead != NULL) {
                    job* nextLastStoppedJob = getLastStoppedStartingAtListEnd();
                    if (nextLastStoppedJob != NULL) {
                        //printf("found nextLastStoppedJob\n");
                        continueJobInBackground(nextLastStoppedJob);
                        return 0;
                    }
                    else {
                        fprintf(stderr,"no stopped jobs\n");
                        return 1;
                    }
                    
                }
                else {
                    fprintf(stderr,"no running jobs\n");
                    return 1;
                } 
            } 
        }
        else {  // user entered a second argument and not just bg
            int jobId = getJobIdFromArg(argv, 1);
            if (jobId == -1) { // not valid
              fprintf(stderr, "invalid job id\n");
                return 1;
            }
            else { // lets find job associated with this jobid and run in background
                if (jobListHead != NULL) {
                    job* job = getJobByJobId(jobId);
                    bool executionPossible = canJobBeExecuted(job);
                    if (executionPossible) {
                        // lets execute it
                        suspendJob(job);
                        continueJobInBackground(job);
                        return 0;
                    }
                    else { // we can't execute
                        fprintf(stderr, "This job has either finished or terminated\n");
                        return 1;
                    }
                }
                else {
                    fprintf(stderr, "no running jobs\n");
                    return 1;
                }
            }    
        }
    }
    else if (strcmp(argv[0], "kill") == 0) { 
        if (argv[1] == 0) { // too few args
            printf("Too few args.\n");
            return 1;
        }
        else if (argv[2] == 0) { // kill and number given 
                int jobId = getJobIdFromArg(argv, 1);
            if (jobId == -1) {
              fprintf(stderr, "This is an invalid job id.\n");
              return 1;
            }
            else {
              updateProcessesAndJobs();
              job* jobToKill = getJobByJobId(jobId);
              // lets check to see if job has not already terminated or completed
              if (jobToKill -> state == -1) {
                fprintf(stderr, "Job can't be killed because it has already been terminated.\n");
                return 1;
              }
              else if (jobToKill -> state == 0) {
                fprintf(stderr, "Job can't be killed because it has completed.\n");
                return 1;
              }
              else { // job is either stopped or running and can be killed
                killJob(jobToKill, 1);
                return 0;
              }
            }
        }
        else if (argv[3] == 0) { // kill and signal and number given 
            //printf("we make it to the top");
            if ((strcmp(argv[2], "-SIGHUP") == 0) || (strcmp(argv[2], "-SIGINT") == 0) || (strcmp(argv[2], "-SIGTERM") == 0) || (strcmp(argv[2], "-SIGKILL") == 0) ) {
                //printf("verified second command\n");
                int signalType = -1;
                signalType++;
                signalType--; 
                if ((strcmp(argv[2], "-SIGHUP") == 0) ){
                    signalType = 0;
                }
                else if (strcmp(argv[2], "-SIGINT") == 0) {
                    signalType = 1;
                }   
                else if (strcmp(argv[2], "-SIGTERM") == 0) {
                    signalType = 2;
                }
                else { // sigkill 
                    signalType = 3;
                }
                //printf("signaltype : %d\n", signalType);
                // 0 == SIGHUP
                // 1 == SIGINT
                // 2 == SIGTERM 
                // 3 == SIGKILL 
                // now try to  kill 
                int jobId = getJobIdFromArg(argv, 2);
                if (jobId == -1) {
                  fprintf(stderr,"This is an invalid job id.\n");
                  return 1;
                }
                else {
                  updateProcessesAndJobs();
                  job* jobToKill = getJobByJobId(jobId);
                  // lets check to see if job has not already terminated or completed
                  if (jobToKill -> state == -1) {
                    fprintf(stderr, "Job can't be killed because it has already been terminated.\n");
                    return 1;
                  }
                  else if (jobToKill -> state == 0) {
                    fprintf(stderr, "Job can't be killed because it has completed.\n");
                    return 1;
                  }
                  else { // job is either stopped or running and can be killed
                    killJob(jobToKill, 1);
                    return 0;
                  }
                }

            }
            else { // invalid second command don't even bother checking third command 
              fprintf(stderr, "invalid second command\n");
                return 1;
            }
        }
        else {
            // too many args given 
          fprintf(stderr, "too many arguments given\n");
            return 1;
        }
    }
    else if (strcmp(argv[0], "jobs") == 0) {
        if (jobListHead != NULL) {
            printJobs();
            return 0;
        }
        else {
            fprintf(stderr, "no jobs\n");
            return 1;
        }
    }
    else {
        // do nothing
        return 0;
    }
    return 0;
}


job* getJobByJobId(int jobId) {
    job* currentJob = jobListHead;
    while (currentJob != NULL) {
        if (currentJob -> jobid == jobId) {
            return currentJob;
        }
        currentJob = currentJob -> nextJob;
    }
    // if we get down here we did not find job
    return NULL;
}

bool canJobBeExecuted(job* job) {
    updateProcessesAndJobs();
    if (job -> state != -1 && job -> state != 0) {
        return true;
    }
    else {
        return false;
    }
}

int getJobIdFromArg(char** argv, int argNumber) {
    int lengthOfSecondArgument = strlen(argv[argNumber]);
    char secondArgBuffer[lengthOfSecondArgument];
    strcpy(secondArgBuffer, argv[argNumber]);
    char* startPositionSecondArg = secondArgBuffer;
    char* endPositionSecondArg = startPositionSecondArg + lengthOfSecondArgument - 1;
    char* currentPositionSecondArg = secondArgBuffer;
    if (*currentPositionSecondArg == '%') {
        // update start position
        startPositionSecondArg++;
        currentPositionSecondArg++;
        while (currentPositionSecondArg <= endPositionSecondArg) {
            if (!isADigit(currentPositionSecondArg)) {
                return -1;
            }
            currentPositionSecondArg++;
        }
        // if we haven't returned up until this point it is a valid number
        int jobId = atoi(startPositionSecondArg);
        return jobId;
        
    }
    else if (isADigit(currentPositionSecondArg)) {
        currentPositionSecondArg++;
        while (currentPositionSecondArg <= endPositionSecondArg) {
            if (!isADigit(currentPositionSecondArg)) {
                return -1;
            }
            currentPositionSecondArg++;
        }
        // if we haven't returned up until this point it is a valid number
        int jobId = atoi(startPositionSecondArg);
        return jobId;
    }
    else {
        return -1;
    }
}



bool isADigit(char* character) {
    if (*character >= '0' && *character <= '9') {
        return true;
    }
    else {
        return false;
    }
}

shell_process* getProcessByPID(pid_t pid) {
    job* currentJob = jobListHead;
    shell_process* currentProcess;
    while (currentJob != NULL) {
        currentProcess = currentJob -> process;
        while (currentProcess != NULL) {
            if (currentProcess -> pid == pid) {
                return currentProcess;
            }
            currentProcess = currentProcess -> next_process;
        }
        currentJob = currentJob -> nextJob;
    }
    // if get to this point this means we did not find process struct
    return NULL;
    
}



void updateProcessesAndJobs() {
    // want to handle all of the buffered pids that stopped, finished, or were terminated in the time since our last pid call
    int status;
    pid_t pid;
    shell_process* process;
    while(((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) && errno != ECHILD) {
        //printf("IN UPDATEPROCESSESANDJOBS WE CAPTURED PID: %d\n", pid);
        process = getProcessByPID(pid);
        if (WIFEXITED(status)) { // finished normally
            process -> finished = true;
        }
        else if (WIFSTOPPED(status)) { // stopped
            process -> stopped = true;
        }
        else if (WIFSIGNALED(status) && ((WTERMSIG(status) == SIGTERM) || WTERMSIG(status) == SIGKILL)) { // terminated
            process -> terminated = true;
        }
        else {
            // do nothing 
        }

    } // end of waitpid while loop, all of the processes should now be updated
    // NOW LETS UPDATE THE JOBS
    //printf("pid: %d", pid);
    //printf("did we make it after waitpid search in update\n");
    job* currentJob = jobListHead;
    bool stopped = true;
    bool finished = true;
    bool terminated = true;
    while (currentJob != NULL) {
        stopped = true;
        finished = true;
        terminated = true;
        currentProcess = currentJob -> process;
        while (currentProcess != NULL) {
            if (currentProcess -> stopped == false) {
                stopped = false;
            }
            if (currentProcess -> finished == false) {
                finished = false;
            }
            if (currentProcess -> terminated == false) {
                terminated = false;
            }
            currentProcess = currentProcess -> next_process;
        }
        // after process check but before incrementing to new job lets update status of job
        if (stopped) {
            lastStoppedJob = currentJob;
            //fprintf(stderr, "job is stopped\n");
            currentJob -> state = 1;
        }
        else if (finished) {
            if(lastStoppedJob == currentJob) {
              lastStoppedJob = NULL;
            }
            if (foregroundJob == currentJob) {
              foregroundJob = NULL;
            }
            //printf("job is finished\n");
            currentJob -> state = 0;
        }
        else if (terminated) {
            if(lastStoppedJob == currentJob) {
              lastStoppedJob = NULL;
            }
            if (foregroundJob == currentJob) {
              foregroundJob = NULL;
            }
            //  printf("job is terminated\n");
            currentJob -> state = -1;

        }
        else {
            currentJob -> state = 2;
        }

        currentJob = currentJob -> nextJob;
    }

}

void markAllProcessesRunning(job* runningJob) {
    shell_process* currentProcess = runningJob -> process;
    while (currentProcess != NULL) {
        currentProcess -> finished = false;
        currentProcess -> terminated = false;
        currentProcess -> stopped = false;
        currentProcess = currentProcess -> next_process;
    }
}

job* getLastStoppedStartingAtListEnd() {
    updateProcessesAndJobs();
    job* currentJob = jobListHead;
    while (jobListHead -> nextJob != NULL) {
        currentJob = currentJob -> nextJob;
    }
    // we are now at tail 
    while ((currentJob -> state != 1) && (currentJob -> prevJob != NULL)) {
        currentJob = currentJob -> prevJob;
    }
    // we should now be at first encountered stopped job when searching from right to left
    if (currentJob -> state == 1) {
        return currentJob;
    }
    else {
       return NULL; 
    }
}


// if (runningJob -> process -> inFile == STDIN_FILENO) {
//                 // display to user what they wrote
//                 write(STDOUT_FILENO, character, 1);
//                 if (runningJob -> process -> outFile != STDOUT_FILENO){ // we have to make sure not to write twice in case where outFile is stdout 
//                     write(runningJob -> process -> outFile, character, 1); // as we already wrote character to stdout
//                 }
//             }
//             else {
//                 // don't write anything the user inputted to outfile because this process
//                 // is taking input from another source
//             }