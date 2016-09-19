#include "320sh.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// Assume no input line will be longer than 1024 bytes





// CREATES THE PROMPT PRINTED TO THE SCREEN
char* getPrompt(char* promptBuffer){

	promptBuffer[0] = '[';
	getcwd(promptBuffer + 1, MAX_INPUT);
	strcat(promptBuffer, "] 320sh> ");

	return promptBuffer;

}

char* getLastCommand(int fp){
	lastCommand(fp);
	char* tempBuff = malloc(MAX_INPUT);
	char character[1];
	int count = 0;

	while(read(fp, character, 1) != 0){
		*(tempBuff + count) = *character;
	}

	return tempBuff;
}

// PLACES FILE POINTER TO THE BEGGINNING OF THE LAST COMMAND
int lastCommand(int fp){

	int rv = 0;

	char character[1];

	off_t cursor = lseek(fp, 0, SEEK_CUR);

	//If we are at the beginning of the file, we need to reposition to the end
	if(cursor == 0){
		lseek(fp, 0, SEEK_END);
		return 1;
	}

	//skip last newLine in file
	lseek(fp, -1, SEEK_CUR);

	while(1){

		//Move to last character
		cursor = lseek(fp, -1, SEEK_CUR);

		//Read it
		read(fp, character, 1);

		//If we hit the NEW LINE character, return the pointer
		if(*character == '\n'){
			return rv;
		}
		//Otherwise, move back one bytes.  Skipping the byte we just read
		else{
			//printf("\n%s\n",character);
			cursor = lseek(fp, -1, SEEK_CUR);
			//We are at the front of the file
			if(cursor == 0){
				return rv; 
			}
		}

	}

}

int nextCommand(int fp){
	char character[1];

	int rv = read(fp, character, 1);

	//If we are at the end of the file, we need to reposition to the start
	if(rv == 0){
		return 1;
	}

	while(1){

		//Read it
		rv = read(fp, character, 1);

		//If we hit the NEW LINE character, return the pointer
		if(*character == '\n'){
			break;
		}
		else if(rv == 0){
			return 1;
		}

	}

	return 0;
}




// READS THE LAST COMMAND AND PLACES POINTER BACK WEHRE IT STARTED
// @returns the new offset on the file
int readCommand(int fp,int buffSize, char* cmd, int type){
	//clear stdout, but only if we have typed something

	char* character = malloc(1);
	
	
	if(type == 0){
		int rv = nextCommand(fp);
		if(rv == 1){
			clearBuffer(cmd, buffSize);
			return 0;
		}
	}
	else if(type == 1){
		lastCommand(fp);
	}


	if(buffSize > 1){
		clearBuffer(cmd, buffSize);
	}

	//set the cursor to where we previously have been

	//off_t place = lseek(fp, 0, SEEK_CUR);
	int count = 0;
	

	read(fp, character, 1);
	while(*character != '\n'){
		write(1, character, 1);
		*(cmd + count) = *character;
		count++;
		int rv = read(fp, character, 1);
		if(rv == 0)
			break;
	}
	//count++;
	lastCommand(fp);

	free(character);
	return count;

}

void clearBuffer(char* cmd, int buffSize){
	while(buffSize > 0){
			write(1, "\b \b", 3);
			*(cmd + buffSize - 1) = '\0';
			buffSize--;
	}
	write(1, cmd, 20);
}

//returns value of the provided variable
int echoBuiltIn(char* variable){

	char firstChar = *variable;
	variable++;

	//Check to see if this is a variable
	if(firstChar == '$'){
		char* value = getenv(variable);
		if(value != NULL){
			fprintf(stdout, "%s\n", value);
			return 0;
		}
	}
	else{
		variable--;
		fprintf(stdout, "%s\n", variable);
		return 0;
		
	}	
	return 1;
}


// Sets environment variable
int setBuiltIn(char** argv){
	//check if the environment variable exists
	if(getenv(argv[1]) != NULL){
		if(strcmp(argv[2], "=") == 0){
			//should run stat on directory
			setenv(argv[1], argv[3], 1);
			return 0;
		}
		else{
			return 1;
		}
	}
	else{
		return 1;
	}

	return 1;
}

//Sets return environment variable
void setReturnVar(int value){
  char strVal = value + '0';
  setenv("?", &strVal, 1);
}

//Dumps contents of the history file
//@return either 0 or 1 for success and failure successfully
int historyDump(){
	int history = open("commandHistory.txt", O_RDWR);
	lseek(history, 0, SEEK_SET);
	char character[1];
	int rv = read(history, character, 1);
	while(rv != 0){
		write(1, character, 1);
		rv = read(history, character, 1);
		//lseek(history, 1, SEEK_CUR);
	}
	return close(history);

}

int clearHistory(){
	FILE* file = fopen("commandHistory.txt", "w");
	if(file == NULL){
		return 1;
	}else{
		fclose(file);
		return 0;
	}
}


void helpBuiltIn(char* command){
	char fileName[20];
	memset(fileName, '\0', 20);
	strcat(fileName, "help/");
	
	strcat(fileName, command);
	strcat(fileName, ".txt");

	//printf("string: %s\n",fileName);

	int fd = open(fileName, O_RDONLY);
	char character[1];

	while(read(fd, character, 1) != 0){
		write(1, character, 1);
	}

	close(fd);
}

/* Creates a list of structs from the array of arguments passed
 * @returns the address of the parent command 
 */
void* structCreation(char* argv[], int numSysArgs){
	//Parent Command, the first to be executed
	shell_process* parentProcess = malloc(sizeof(struct shell_process));
	processHead = parentProcess;
	shell_process* process = parentProcess;
	process -> argv = NULL;
	process -> next_process = NULL;
	process -> inFile = STDIN_FILENO;
	process -> outFile = STDOUT_FILENO;
	process -> stopped = false;
	process -> finished = false;
	process -> terminated = false;
	int count = 0;
	int tempFile;
	//char* argvTemp = argv[0];

	if(isCommand(argv[count]) == true){						//Check if first arg is a command
		//printf("\ncommand:%s\n", argv[count]);
		process -> argv = argv + count;
		if(strcmp(argv[count], "help") == 0){
			//printf("here\n");
			return process;
		}
	}
	else if((tempFile = isFile(argv[count])) != -1){		//Check is first arg is a file
		count++;
		if(strcmp(argv[count] , "<") == 0){					// Input was found
			argv[count] = "\0";
			process -> outFile = tempFile;
		}
		else if(strcmp(argv[count] , ">") == 0){			// Output was found
			argv[count] = "\0";
			process -> inFile = tempFile;
		}
		else{												//Parsing error, return NULL
			return NULL;
		}
	}
	else{													//If its nothing else, parsing error
		return NULL;
	}

	count++;												//Start one ahead of where we were

	while(argv[count] != NULL){
		//printf("\n%d\t%s\n",count, argv[count]);
		if(isCommand(argv[count]) == true){						//command was found
			//printf("\ncommand:%s\n", argv[count]);
			process -> argv = argv + count;						
		}
		else if(strcmp(argv[count] , "<") == 0 && process -> argv != NULL) {					// Input was found
			//printf("\nhere: %s\n",argv[count]);
			argv[count++] = 0;
			process -> inFile = isFile(argv[count]);
			if(process -> inFile < 0){					//Parsing error, return NULL
				//printf("\nbad1\n");
				return NULL;
			}
		}
		else if(strcmp(argv[count] , "<") == 0 && process -> argv == NULL) {					// output was found
			//printf("\nhere: %s\n",argv[count]);
			argv[count++] = 0;
			process -> outFile = isFile(argv[count]);
			if(process -> outFile < 0){					//Parsing error, return NULL
				//printf("\nbad1\n");
				return NULL;
			}
		}
		else if(strcmp(argv[count] , ">") == 0 && process -> argv != NULL){			// Output was found
			argv[count++] = 0;
			process -> outFile = isFile(argv[count]);
			if(process -> outFile < 0){					//Parsing error, return NULL
				//printf("\nbad2\n");
				return NULL;
			}
		}
		else if(strcmp(argv[count] , ">") == 0 && process -> argv == NULL){			// input was found
			argv[count++] = 0;
			process -> inFile = isFile(argv[count]);
			if(process -> inFile < 0){					//Parsing error, return NULL
				//printf("\nbad2\n");
				return NULL;
			}
		}
		else if(strcmp(argv[count], "|") == 0){				//Pipe was found, we need to create a new command
			argv[count] = 0;
			process -> next_process = malloc(sizeof(struct shell_process));
			process = process -> next_process;
			process -> next_process = NULL;
			process -> argv = NULL;
			process -> inFile = STDIN_FILENO;
			process -> outFile = STDOUT_FILENO;
			count++;
			bool nextProcess = isCommand(argv[count]);
			if(nextProcess == false){							//Parsing error, return NULL
				//printf("\nbad3\n");
				return NULL;
			}
			else{
				continue;
			}

		}
		count++;
	}
	if(process -> argv == NULL){
		process -> argv = argv + count - 1;
	}

	//fprintf(stdout, "\n%s\n",argv[command -> command]);

	//exit(0);

	processTail = process;

	return parentProcess;
}

bool isCommand(char* command){

	char* builtInCommands[12] = {"cd", "pwd", "echo", "set", "help", "exit", "history", "clear-history", "jobs", "kill", "fg", "bg"};

	if(strchr(command, '/') != NULL){
		//printf("\nfile\n");
  		struct stat fileStat;
  		if(stat(command, &fileStat) < 0){
  			return false;
  		}else{
  			true;
  		}
  	}
  	else if (isBuiltInCommand(command, builtInCommands) != -1){	
  		//printf("\nbuilt in\n");			
  		return true;
  	}
  	else if (getProgramPath(command) != NULL && strcmp(command, "..") != 0 && strcmp(command, ".") != 0){
  		//printf("\nPATH\n");
  		return true;
  	}


	return false;
}

int isFile(char* fileName){
	if(strstr(fileName, ".txt") != NULL)
		return open(fileName, O_RDWR | O_CREAT, S_IROTH | S_IWOTH | S_IRUSR | S_IWUSR);
	else
		return -1;
}


// STRTOK CHOPS UP OUR ORIGINAL CMDBUFFER BY PUTTING 0'S EVERYWHERE
// This was fine when we were only searching through the entered command line once
// But now that we are doing it multiple times to get the count, length of longest arg, and once more
// To fill a sysArgs array, we must make a copy of cmdBuffer and use that instead of using the original
// .

void execProg(char* cmdBuffer, bool debug){

	if(strcmp(cmdBuffer, "exit") == 0){
		exitBuiltIn();
	}

  //Print to stderr to show were running a command
	if(debug == true){
		fprintf(stderr, "RUNNING: %s\n",cmdBuffer);
	}

	char cmdBufferDup[MAX_INPUT];
	// make a copy of cmdBuffer
	memset(cmdBufferDup, '\0', MAX_INPUT);
	memcpy(cmdBufferDup, cmdBuffer, MAX_INPUT);
	int numSysArgs = getNumSysArgs(cmdBufferDup);
	  
	// make a copy of cmdBuffer
	memset(cmdBufferDup, '\0', MAX_INPUT);
	memcpy(cmdBufferDup, cmdBuffer, MAX_INPUT);
	int longestArg = lenLongestSysArg(cmdBufferDup);

	// make a copy of cmdBuffer
	memset(cmdBufferDup, '\0', MAX_INPUT);
	memcpy(cmdBufferDup, cmdBuffer, MAX_INPUT);
	// this is the start address of the "two-dimensional" array containing our sys args
	char* sysArgs = getAllSysArgs(cmdBufferDup, numSysArgs, longestArg);
	// however the second parameter in execv(), char *const argv[], calls for an array of char pointers, so we must make this array of pointers and fill it 
	char* argv[numSysArgs + 1]; // need one extra to hold NULL at end marking the end of the array
	// fill the array
	int cursor = 0;
	for (int i = 0; i < numSysArgs; i++) { 
	    argv[i] = sysArgs + cursor;   
		cursor += (longestArg+2); // +1 based on getAllSysArgs()'s' storage implementation where each sysArgs element is guaranteed to be null terminated
	                              // this is required implementation as argv[] too has to have each element null terminated
	}
	// now that we've constructed the sysArgs[] array we must replace any NAKS that were inserted
	// which was part of the process of supporting quotations in our shell
	// ********************** REMOVING NAKS ************************************
	for (int i = 0; i < numSysArgs; i++) {
	    char replaceSpace[1];
	    char* replaceLocation;
	    char* cursor = argv[i];
	    int length = strlen(argv[i]);
	    char* lastCharacterLocation = cursor + length -1;
	    while ((replaceLocation = strchr(cursor, 21)) != NULL && replaceLocation <= lastCharacterLocation) {
	    	*replaceSpace = ' ';
	    	*replaceLocation = *replaceSpace;
	    	cursor = replaceLocation + 1;
	    }
	}

	// make a copy of cmdBuffer
	memset(cmdBufferDup, '\0', MAX_INPUT);
	memcpy(cmdBufferDup, cmdBuffer, MAX_INPUT);
	  

	// set last pointer in argv[] to 0 for NULL
	argv[numSysArgs] = 0;
	// at this point argv should be full of pointers to each sysArg and we should be ready to go

	// get first command
	//char* command = getNextSysArg(cmdBuffer);
	

	structCreation(argv, numSysArgs);

	shell_process* currentProcess = processHead;

	int r = 0;

	if(strcmp(argv[0], "jobs") == 0 || strcmp(argv[0], "kill") == 0 || strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0){
		// printf("Second is : %s\n", argv[1]);
		// printf("Third is : %s\n", argv[2]);
		// printf("Fourth: %p\n", argv[3]);
		// printf("FIFTH: %p\n", argv[4]);
		r = handleJobsRequest(argv);
		setReturnVar(r);
		return;
	}

	while(currentProcess != NULL){
		if(currentProcess -> argv == NULL){
			fprintf(stderr, "command not found\n");
			setReturnVar(1);
			return;
		}
		currentProcess = currentProcess -> next_process;
	}

	int commandNum;
	char* command = processHead -> argv[0];
	char* builtInCommands[12] = {"cd", "pwd", "echo", "set", "help", "exit", "history", "clear-history", "jobs", "kill", "fg", "bg"};

	if ((commandNum = isBuiltInCommand(command, builtInCommands)) != -1) { // THIS IS A BUILT IN COMMAND 
	// 0 = "cd" | 1 = "pwd" | 2 = "echo" | 3 = "set" | 4 = "help" | 5 = "exit"
	//printf("commandNum: %d\n", commandNum);
		switch (commandNum) {
	  		case 0: {    // cd
		        char cdArgsDup[MAX_INPUT];
		        // first check to make sure we can access argv[1] meaning something followed cd
		        if (processHead -> argv[1] == 0) { // this is where I will handle when user typed just cd followed by no arguments
		          	processHead -> argv[1] = processHead -> argv[0] + 4;
		          	*(processHead -> argv[1]) = '!';
		        }
		        // NOW LETS SEE IF THERE ARE ANY SLASHES IN THE ARGUMENT FOLLOWING CD
		        char* slashLocation = strchr(processHead -> argv[1], '/');

		        if (slashLocation != NULL) {
		          	// special case where we have a slash as the first thing beginning argv[1]
		          	// Lets say user typed cd /usr/bin
		          	// If we were to try to delemit "/" on "/usr/bin" (currently stored in argv[1]) strtok would return null
		          	// So we must shift the contents of argv[1] 1 byte over in memory, and write a " " at the first byte position
		          	// So that when we delimit argv[1] using "/" we get 
		          	// cdArray[0] = " "
		          	// cdArray[1] = "usr"
		          	// cdArray[2] = "bin"
		          	// This has been made possible by the getAllSysArgs() null terminating with TWO null characters at the end to allow for the shift if needed
		          	if (slashLocation == processHead -> argv[1]) {
		            	// first shift everything one byte over
		            	int length = strlen(processHead -> argv[1]);
		            	memmove(slashLocation+1, slashLocation, length);
		            	// write " " at slashLocation (beginning of argv[1])
		            	char spaceSpace[1];
		            	*spaceSpace = ' ';
		            	*processHead -> argv[1] = *spaceSpace;
		          	}

		          	memset(cdArgsDup, '\0', MAX_INPUT);
		          	memcpy(cdArgsDup, processHead -> argv[1], MAX_INPUT);
		          	int numcdArgs = getNumcdArgs(cdArgsDup);

		          	memset(cdArgsDup, '\0', MAX_INPUT);
		          	memcpy(cdArgsDup, processHead -> argv[1], MAX_INPUT);
		          	int longestcdArg = lenLongestcdArg(cdArgsDup);

		          	memset(cdArgsDup, '\0', MAX_INPUT);
		         	memcpy(cdArgsDup, processHead -> argv[1], MAX_INPUT);
		          	// this is the start address of the "two-dimensional" array containing our cd args
		          	char* cdArgs = getAllcdArgs(cdArgsDup, numcdArgs, longestcdArg);
		          	// however I want them to be in an array of type char *const, an array of char pointers (easy to work with)
		          	char* cdArray[numcdArgs + 1]; // need one extra to hold NULL at end marking the end of the array
		          	// fill the array
		          	int cdCursor = 0;
		          	for (int i = 0; i < numcdArgs; i++) { 
		            	cdArray[i] = cdArgs + cdCursor;   
		            	cdCursor += (longestcdArg+1); // +1 based on getAllSysArgs()'s' storage implementation where each sysArgs element is guaranteed to be null terminated
		                                      // this is required implementation as argv[] too has to have each element null terminated
		          	}
	      			// set last pointer in argv[] to 0 for NULL
	      			cdArray[numcdArgs] = 0;
	      			// at this point cdArray should be full of pointers to each cdArg and we should be ready to go
	      			// NOW CALL cdBuiltIn
	      			r = cdBuiltIn(cdArray);
	      			free(cdArgs);
	    		}
	    		// IF THERE ARE NO SLASHES IN THE ARGUMENT FOLLOWING CD THEN JUST PUT THE WHOLE THING IN cdArray[0]
	    		else { 
	      			char* cdArray[2];
	      			//printf("fekwoipfkewopfw\n");
	      			cdArray[0] = processHead -> argv[1];
	      			cdArray[1] = 0;
	      			// NOW CALL cdBuiltIn

	      			r = cdBuiltIn(cdArray);
	    		}
	    		break;
	  		}
	  		case 1: {    // pwd
	    		char currentDir[MAX_INPUT];
	    		r = pwdBuiltIn(currentDir);
	    		break;
	  		}
	  		case 2: {   // echo
	  			r = echoBuiltIn(processHead -> argv[1]);
	  			setReturnVar(r);
	  			if(debug == true){
	    			fprintf(stderr, "ENDED: %s ret=%d\n",cmdBuffer, r);
	  			}
	    		break;
	  		}

	  		case 3: {    // set
	  			r = setBuiltIn(processHead -> argv);
	  			break;
	  		}
	  		case 4: {    // helpBuiltIn
	  			//printf("sys: %d\n",numSysArgs);
	  			if(((commandNum = isBuiltInCommand(argv[1], builtInCommands)) != -1) || numSysArgs == 1){
					//printf("not here\n");
					if(numSysArgs > 1){
						//printf("not here\n");
						helpBuiltIn(argv[1]);
					}else if (numSysArgs == 1){
						//printf("\nhere\n");
						char* mainStr = "main";
						helpBuiltIn(mainStr);
					}
					r = 0;
				}
	    		break;
	  		}

	  		case 5: {   // exit
	    		// obviously we'll have to do some cleaning up later of jobs and such here
	    		exitBuiltIn();

	  			break;
	     	}
	  		case 6: {	 //history
	  			r = historyDump();
	  			break;
	  		}

	  		case 7: {	 //clear-history
	  			r = clearHistory();
	  			break;
	  		}
		}
		setReturnVar(r);
		if(debug == true){
	    	fprintf(stderr, "ENDED: %s ret=%d\n",cmdBuffer, r);
	  	}
		return;
	} 

	char* commandCpy = malloc(strlen(cmdBuffer) + 1);
	memset(commandCpy, '\0', strlen(cmdBuffer) + 1);
	memcpy(commandCpy, cmdBuffer, strlen(cmdBuffer));

	job* startJob = createJob(argv[0], commandCpy, processHead, processHead -> inFile, processTail -> outFile);
	startJob -> pgid = 0;

	//printf("last arg: %s\n", argv[numSysArgs -1]);

	// here we must check if user wants 
	if (*argv[numSysArgs - 1] != '&'){
		//printf("executing in foreground\n");
		executeNewJobInForeground(startJob, debug);
	}
	else {
		argv[numSysArgs - 1] = 0;
		executeNewJobInBackground(startJob, debug);
	}

  	// FREE MEMORY RESERVED FOR SYSARGS, don't forget to include freeing if it was a built in 
  	free(sysArgs);
}

int execProcess(shell_process* process, bool debug, char* cmdBuffer){
	
	int rv = 0;


	char* builtInCommands[12] = {"cd", "pwd", "echo", "set", "help", "exit", "history", "clear-history", "jobs", "kill", "fg", "bg"};
	int commandNum;	 	
	char* command = process -> argv[0];

	//Check to see if it contains a '/', if so we can pass directly to the exec function
	if(strchr(command, '/') != NULL){
		struct stat fileStat;
		if(stat(command, &fileStat) < 0){
			fprintf(stderr, "%s: file does not exist\n", command);
			return 1;
		}
		
  		//Child process
  		rv = execv(command, process -> argv);
  		if(rv == -1){
  			return 1;
    		fputs("something went wrong\n",stderr);
  		}
		
	}
	else if ((commandNum = isBuiltInCommand(command, builtInCommands)) != -1) { // THIS IS A BUILT IN COMMAND 
	// 0 = "cd" | 1 = "pwd" | 2 = "echo" | 3 = "set" | 4 = "help" | 5 = "exit"
	//printf("commandNum: %d\n", commandNum);
		switch (commandNum) {

	  		case 2: {   // echo
	  			int ret = echoBuiltIn(process -> argv[1]);
	  			setReturnVar(ret);
	  			if(debug == true){
	    			fprintf(stderr, "ENDED: %s ret=%d\n",cmdBuffer, ret);
	  			}
	    		break;
	  		}
		}
	}

	else{
		
		//Child process
		char* programPath = getProgramPath(process -> argv[0]);
		if (programPath != NULL) {
			rv = execv(programPath, process -> argv);
			if(rv == -1){
				return 1;
			}else{
			
			}
			free(programPath);
		}
		else {

			return 1;
			fprintf(stderr, "%s: command not found\n", process -> argv[0]);
		}

	}
	//printf("we made it to child exit");
	exit(rv);
}