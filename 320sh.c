#include "320sh.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

// Assume no input line will be longer than 1024 bytes
// #define MAX_INPUT 1024

int
main (int argc, char ** argv, char **envp) {


  // shellPGID = getpgid(getpid());
  // printf("SHELLPID: %d\n", shellPGID);

  int finished = 0, opt;
  char cmd[MAX_INPUT];
  memset(cmd, '\0', MAX_INPUT);
  char last_command[MAX_INPUT];
  memset(last_command, '\0', MAX_INPUT);
  char currentDir[MAX_INPUT];
  char charCheck[1];
  bool enterPressed = false;
  bool tabPressed = false;
  debug = false;




  // while ((shellPGID = getpgrp()) != tcgetpgrp(STDIN_FILENO)){
  //   kill (shellPGID, SIGTTIN);
  // }
  //setpgid(getpid(), getpid());
  //tcsetpgrp (STDIN_FILENO, getpid());

  //setvbuf(stdout, cmd, _IOFBF, 1024);

  //check to see if we should debug
  while((opt = getopt(argc, argv, "d")) != -1) {
    switch(opt) {
      case 'd':
        debug = true;
      break;
    }
  }

  //Initialize the return value environment variable
  setenv("?", "0", 1);

  int output = open("output.txt", O_RDWR);
  int history = open("commandHistory.txt", O_RDWR);
  //move pointer to end of file and to last command

  memcpy(last_command, getLastCommand(history), MAX_INPUT);
for_loop:
  while (!finished) {

    char *cursor;
    char last_char;
    char* prompt = getPrompt(currentDir);
    int rv;
    int count;

    // Print the prompt
    //sleep(2);
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }

    fcntl(0, F_SETFL, O_APPEND);

    for(rv = 1, count = 0, 
    cursor = cmd, last_char = 1;
    rv
    && (count < (MAX_INPUT-1))  // if our MAX_INPUT IS 1024, this line says we stop at 
    && (last_char != '\n');       // counter = 1022 (before increment) as counter++ = 1023
    cursor++) {                   // as 1023 is not < 1023. We read in a max of 1022 max characters. Why not 1024? (I borrowed this from given code)
      rv = read(0, charCheck, 1);
      // check last character
      last_char = *charCheck;
      //printf("%d\n",last_char);

      // count at this point is actually 1 more than number of bytes we have written to buffer so subtract one
      // if (count > 0){
      //   char* lastCharPosition = cmd + count -1;
      //   if (cursor <= lastCharPosition && last_char != '\n'){ // don't want to shift bytes if hitting enter
      //     // printf("we entered if\n");
      //     shiftBufferContents(cmd, cursor, last_char, lastCharPosition); 
      //   }
      // }

      if (last_char == '\n') {  // new line, user hit enter
        // handling special case where user presses enter but there is no command to execute
        write(1, "\n", 1);
        if (cursor == cmd) {
          //printf("%d\n", count);
          //printf("%p\n", cursor);
          break;
        }
        else { // there is a command to process so proceed as usual
          enterPressed = true;
          char* lastCharPosition = cmd + count -1;
          if (cursor <= lastCharPosition){ // do nothing, we don't want to null terminate if we are somewhere in the middle of the buffer

          }
          else { // else we are at the end so lets null terminate buffer
            *cursor = '\0';
          }
          //*cursor = '\0';
          //count--;
        }
      }
      else if (last_char == 127) { // user hit backspace
        if (cursor > cmd) { // only want to move cursor back and write null if we won't step out of the bounds of the buffer
          char* lastCharPosition = cmd + count -1;
          if (cursor <= lastCharPosition){ // CURSOR IS SOMEWHERE IN MIDDLE OF ARRAY
            shiftBufferUpdateSTDOUT(cmd, cursor, cursor-1, ' ', lastCharPosition);
            *lastCharPosition = '\0';
            count--;
            cursor = cursor -2;
          }
          else {
            write(1, "\b \b", 3);
            cursor = cursor - 1;
            *cursor = '\0';
            cursor = cursor - 1;
            count--;
          }
        }else{
          cursor--;
          //count--;
        }
      }
      else if (last_char == '\t') { // user hit tab
        tabPressed = true;
        *cursor = '\0';
      }
      else if (last_char == 27){ // must have used and arrow key
        rv = read(0, charCheck, 1);
        last_char = *charCheck;
        if(last_char == '['){
          rv = read(0, charCheck, 1);
          last_char = *charCheck;
          switch(last_char){
            //Up Arrow
            case 'A':             
              count = readCommand(history, count, cmd, 1);
              cursor = cmd + count - 1;
              char temp = count + '0';
              write(output, &temp  , 1);
              write(output, cmd, MAX_INPUT);
              break;
            //Down Arrow
            case 'B':
              count = readCommand(history, count, cmd, 0);
              cursor = cmd + count - 1;
              break;
            //Right Arrow
            case 'C':
              if(cursor <= (cmd + count - 1)){
                write(1, cursor, 1);
              }else{
                cursor--;
              }
              //if()
              break;
            //Left Arrow
            case 'D':
              if(cursor > cmd){
                write(1, "\b", 1);
                cursor-=2;
              }
              else {
                cursor--;
              }
              //count--;
            break;
          }
        }
      }
      else if(last_char == 3){
        cursor--;
        if (foregroundJob != NULL) {
          //printf("foregroundJob is NOT NULL\n");
          killJob(foregroundJob, 1);
          clearBuffer(cmd, count + 1);
          goto for_loop;
        }
        else {
          //printf("foregroundJob IS NULL\n");
          clearBuffer(cmd, count + 1);
          goto for_loop;
        }
      }
      else if(last_char == 26){
        cursor--;
      }
      else if(last_char == 1){
        int temp = count;
        while(temp > 0){
          write(1, "\b", 1);
          temp--;
        }
        cursor = cmd - 1;
      } 
      else { // else is just normal character so lets write it to our buffer
        if (!enterPressed){
          if (count > 0){
            char* lastCharPosition = cmd + count -1;
            if (cursor <= lastCharPosition){ // CURSOR IS SOMEWHERE IN MIDDLE OF ARRAY
              // WE WILL BE SHIFTING 1 BYTE TO THE RIGHT
              // this handles std write as well
              shiftBufferUpdateSTDOUT(cmd, cursor, cursor + 1, last_char, lastCharPosition); 
              //*cursor = last_char; // write character to buffer
            }
            else {
              // we will handle std write explicitly
              write(1, &last_char, 1);
            }
            *cursor = last_char; // write character to buffer
            count++;
          }
          else {
            write(1, &last_char, 1);
            *cursor = last_char; // write character to buffer
            count++;
          }         
        }
        // write(1, cursor, 1);
      }
    } // end of for loop

    // our old character checking code we can delete this whenever
    //   // read 1 byte from stdin
    //   rv = read(0, cursor, 1); 
    //   last_char = *cursor;
    //   printf("%d\n", last_char);
    //   if (last_char == '\n') {  // we hit new line
    //     enterPressed = true;
    //   }else if(last_char == '\t'){

    //   }
    // } // end of for loop
    if (tabPressed) {
      // do nothing for now
    }
    // Now lets check if an entire line was given (user printed enter) or if we just ran out of space
    if (enterPressed  && cmd[0] != '\0') {
      //moves history pointer to end of file
      //lseek()
      //writes command to history file
      lseek(history, 0, SEEK_END);
      if(strcmp(last_command, cmd) != 0){
        write(history, cmd, count);

        memcpy(last_command, cmd, MAX_INPUT);
        write(history, "\n", 1);
      }
      
      //printf("offset: %d\n",offset);
      //printAllSysArgs(cmd);

      NAKQuoteSupport(cmd);
      // lets print the arguments to make sure it replaced properly
      //printf("length: %d\n", count);
      printAllSysArgs(cmd);
      execProg(cmd, debug);
      memset(cmd, '\0', MAX_INPUT);
      //goto for_loop;
    }
    else {
      // some kind of error nonsense, user just kept typing and typing and we ran out of room in our buffer
    }



  // ************** THIS WAS THE PROVIDED CODE THAT ONLY ACCOMPLISHES ECHO FUCNTIONALITY ***************** 
  //   // read and parse the input
  //   for(rv = 1, count = 0, 
	 //  cursor = cmd, last_char = 1;
	 //  rv 
	 //  && (++count < (MAX_INPUT-1))
	 //  && (last_char != '\n');
  // 	cursor++) { 
  //     // read 1 byte from stdin
  //     rv = read(0, cursor, 1);  // we hang on this line until user hits enter
  //     last_char = *cursor;
  //     // DECIMAL 3 is the end of text character, '^c'
  //     if(last_char == 3) {
  //       // **** For normal inputs we don't enter this if block,
  //       // I don't exact know when ^c shows up in stdin (haven't encounted that case yet)
  //       write(1, "^c", 2); 
  //     } 
  //     else {
  //     	write(1, &last_char, 1); // write last read-in character to stdout 
  //     }
  //   }// end of for loop 
  //   *cursor = '\0';

  //   if (!rv) { 
  //     finished = 1;
  //     break;
  //   }
  // *****************************************************************************************************


    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now
    // write(1, cmd, strnlen(cmd, MAX_INPUT));

    // reset enterPressed 
    enterPressed = false;
    tabPressed = false;
  } // end of while loop
  int rc = close(history);
  if(rc == 0){
    //fprintf(stdout,"history closed successfully");
  }
  return 0;
}



