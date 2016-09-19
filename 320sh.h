#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


//////////////////////////////////// OUR STRUCTS START 
struct shell_process {
  pid_t pid;
  char **argv;
  int inFile;
  int outFile;
  bool finished;
  bool stopped;
  bool terminated;
  struct shell_process *next_process;
};

struct job {
	pid_t pgid;
	int jobid;
	char* argv;
	char* enteredCommandLine; 
	struct shell_process *process;
	struct job *nextJob;
	struct job *prevJob;
  int inFile;
  int outFile;
  int state; // -1 is terminated, 0 is finished, 1 is stopped, 2 running 
};
/////////////////////////////////// OUR STRUCTS END 

typedef struct shell_process shell_process;
typedef struct job job;

//////////////////////////////////// OUR VARIABLES
shell_process* currentProcess;
shell_process* processHead;
shell_process* processTail;
job* jobListHead;
job* foregroundJob;   // useful for knowing which pgid to suspend orkill
job* lastStoppedJob;  // useful for when fg or bg is typed on its own
bool debug;
//int jobIdCounter;

#define MAX_INPUT 1024
#define OUTPATH "output"

pid_t shellPGID;

//Konrad's Functions
void printAllSysArgs(char* cmdBuffer);
char* getNextIndividualPath(char* cmdBuffer);
char* getNextSysArg(char* cmdBuffer);
int getNumSysArgs(char* cmdBuffer);
int lenLongestSysArg(char* cmdBuffer);
char* getAllSysArgs(char* cmdBuffer, int numSysArgs, int lenLongestSysArg);
char* getProgramPath(char* programName);
int isBuiltInCommand(char* command, char** builtInCommands);
int pwdBuiltIn(char* buffer);
void exitBuiltIn();
int cdBuiltIn(char *const cdArray[]);
int getNumcdArgs(char* cdBuffer);
int lenLongestcdArg(char* cdBuffer);
char* getAllcdArgs(char* cdBuffer, int numcdArgs, int lenLongestcdArg);
void NAKQuoteSupport(char* cmdBuffer);
void shiftBufferUpdateSTDOUT(char* cmdBuffer, char* cursor, char* destinationStart, char characterEntered, char* lastCharPosition);
job* createJob(char* argv, char* cmdLineBuffer, shell_process *firstProcess, int inFile, int outFile);
void addJobToList(job *newJob);
void unblockChildSignals();
void executeJob(job *jobToBeExecuted, bool foreground, bool debug);
void waitForForegroundJobToFinish(job *runningJob);
void executeNewJobInForeground(job *jobToBeExecuted, bool debug);
void continueJobInForeground(job *jobToBeContinued);
void executeNewJobInBackground(job *jobToBeExecuted, bool debug);
void continueJobInBackground(job *jobToBeContinued);
void waitForForegroundJobToCease(job *runningJob);
void killJob(job* jobToKill, int status);
void removeJob(job* jobToRemove);
void printJobs();
int handleJobsRequest(char** argv);
bool canJobBeExecuted(job* job);
int getJobIdFromArg(char** argv, int argNumber);
bool isADigit(char* character);
job* getJobByJobId(int jobId);
shell_process* getProcessByPID(pid_t pid);
void updateProcessesAndJobs();
void markAllProcessesRunning(job* runningJob);
void suspendJob(job* jobToSuspend);
job* getLastStoppedStartingAtListEnd();


//Charles' Functions
void clearBuffer(char* cmd, int buffSize);
char* getPrompt(char* promptBuffer);
int echoBuiltIn(char* variable);
void execProg(char* cmdBuffer, bool debug);
int execProcess(shell_process* process, bool debug, char* cmdBuffer);
char* getLastCommand(int fp);
int historyDump();
bool isCommand(char* command);
int isFile(char* fileName);
int lastCommand(int fp);
int nextCommand(int fp);
int readCommand(int fp, int buffSize, char* cmd, int type);
int setBuiltIn(char** argv);
void setReturnVar(int value);
void* structCreation(char* argv[], int numSysArgs);