#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


/* Global Constants */
double 	DEFAULT_TIME_SLICE		= 1; // Seconds (rounded down to second unless <1s )

int 	DEFAULT_BUFFER_SIZE	 	= 1024;
int 	MAXIMUM_PROCESS_COUNT 	= 256; 



/* Create LinkedList of pids, nodes are called p_links*/
typedef struct p_link {
	pid_t pid;
	struct p_link * next;
} p_link;

/* Boolean */
typedef enum { false, true } bool;


// FUNCTION TABLE
void 		executeChildAndExit			( char * cmd[], int processCount );
bool 		isProcessComplete			( pid_t pid );
void 		SIGALRM_handler				( int pid );
p_link *	createCircularPidLinkList	( pid_t *pids, int processCount );
void 		executeProcess				( pid_t pid, double time_slice );
void 		roundRobinProcessExecution	( pid_t * pids, int processCount );
FILE *		getProcFileForPid			( pid_t pid, char * extension );
void 		getProcStatusData			( FILE * procFile );
void 		getProcSchedData			( FILE * procFile );


/* Ghost Of The MCP Main ***************************************
**
**
***************************************************************/
int main( int argc, char * argv[] ) {

	FILE * 	file 		= NULL;


	if ( !argv[1] ) {
		printf("Second argument missing. Stopping...");
		exit( -1 );
	}


	// Exit program if file open is unsuccessful
	file = fopen( argv[1], "r" );

	if ( file == NULL ) {
		perror("File not found.\n");
		exit( -1 );
	}


	// Setup process id storage for later use
	pid_t 	pids		[DEFAULT_BUFFER_SIZE];

	int 	processCount 	= 0;



	// Setup variables for reading lines from file
	char * 	readLine	= NULL;
	size_t 	readLength 	= 0;
	ssize_t len 		= 0;
	
	// Updates readLine with next line from file while there is one.
	while ((readLength = getline(&readLine, &len, file)) != -1) {


		int 	cmdIndex 	= 0;
		char * 	cmdElement	= NULL;
		char * 	cmdElementsArr	[DEFAULT_BUFFER_SIZE];

	
		pid_t 	childProcessID	= -1;
		

		// Strip newline off of line
		strtok( readLine, "\n" );


		// Grab substring from our readLine up to first space
		cmdElement = strtok( readLine, " " );

		while ( cmdElement != NULL ) {
			cmdElementsArr[cmdIndex++] = cmdElement;
			cmdElement = strtok( NULL, "\n " );
		}
		cmdElementsArr[ cmdIndex ] = 0;


		
		childProcessID = fork();

		if (childProcessID == 0) { // Child Process
			executeChildAndExit( cmdElementsArr, cmdIndex );
			memset( cmdElementsArr, 0, cmdIndex );
		}
		else if (childProcessID < 0) { // Parent Process
			abort();
		}

		pids[ processCount++ ] = childProcessID;

	}
	// Parent Code Only Past Here

	
	// Free that memory!
	free( readLine );
	readLine = NULL;

	fclose( file );
	file 	 = NULL;


	// This sleep is necessary 
	sleep( 1 );


	// Round Robin execution of processes.
	roundRobinProcessExecution( pids, processCount );



	
	// As the parent we must now wait for all our child processes to end.
	int processIndex;
	for ( processIndex = 0; processIndex < processCount; processIndex++ ) {

		int returnStatus;
		waitpid( pids[processIndex], &returnStatus, 0 );

	}
	
	printf( "PROGRAM END.\n" ); // TRAILER SPACING

	return 0;

}



/* Excute Child And Exit ***************************************
** 
** Executes the process stored in cmd[0] with any other arguments
** stored in the rest of cmd's array. Will exit the child process
** as well.
** 
** @param cmd : Array of which process to call and any arguments
** @return [void]
** 
***************************************************************/
void executeChildAndExit(char * cmd[], int processCount) {

	
	// Create signal set
	sigset_t signal_set;

	sigemptyset( &signal_set );
	sigaddset( &signal_set, SIGUSR1 );
	sigprocmask( SIG_BLOCK, &signal_set, NULL );

	// Wait for SIGUSR1
	int wait_return;
	sigwait( &signal_set, &wait_return );

	// Execute the command after a SIGUSR1 is recieved
	
	execvp( cmd[0], cmd );
	exit( 0 );

}



/* Process Completion Status ***********************************
** 
** Check if the supplied process is complete
** 
** @param pid_t pid : pid of the process to check
** @return bool (custom type)
**
***************************************************************/
bool isProcessComplete(pid_t pid) {
	
	int childStatusValue, status;
	childStatusValue = waitpid( pid, &status, WNOHANG );

	return childStatusValue == 0 ? false : true;
}



/* SIGALRM Handler *********************************************
** 
** SIGALRM handler function, also restarts the signal
** 
** @param int pid : pid calling the function
** @return [void]
**
***************************************************************/
void SIGALRM_handler(int pid) {
	// Reset alarm handler
	signal( SIGALRM, SIGALRM_handler );
}



/* createCirualPidLinkList ************************************
** 
** Creates a p_link list of links.
** 
** @param pids[] : array of pids
** @return p_link * : returns one node of circular linked
** list of pids.
**
***************************************************************/
p_link *createCircularPidLinkList(pid_t *pids, int processCount) {

	// Configure a LinkedList with our processes
	p_link * tail; // Tail 

	tail = (p_link * ) malloc( sizeof( p_link ) );
	tail -> pid = pids[ processCount - 1 ];
	tail -> next = NULL; // will be head.

	p_link * next_node = tail;
	p_link * node; // Middle pids
	int node_index;
	for (node_index = processCount - 2; node_index >= 0; node_index--) {
		node = (p_link * ) malloc( sizeof( p_link ) );
		node -> pid  = pids[ node_index ];
		node -> next = next_node;

		next_node = node;
	}
	node = next_node;

	tail -> next = node; // Link the list

	/* Link List created with handle 'node' */

	return node;
	
}



/* Execute Process *********************************************
** 
** Run process for a specified time.
** 
** @param pid_t pid : the process to run
** @return [void]
**
***************************************************************/
void executeProcess(pid_t pid, double sleepLength) {

	kill( pid, SIGCONT );

	signal( SIGALRM, SIGALRM_handler );

	printf("# PROCESS META DATA\n");
	FILE * procFile = NULL;
	procFile = getProcFileForPid( pid, "status" );
	getProcStatusData( procFile );
	fclose( procFile );

	FILE * schedFile = NULL;
	schedFile = getProcFileForPid( pid, "sched" );
	getProcSchedData( schedFile );
	fclose( schedFile );
	printf("# END PROCESS DATA\n");

	// Sleep for the appropriate time
	sleepLength < 1 ? ualarm( (int)(sleepLength * 1000000) % 1000000, 0 ) : alarm( sleepLength );
	pause();

	kill( pid, SIGSTOP );

}



/* Process Round Robin *****************************************
** 
** Send kill signals to turn on and off the processes in a 
** round robin style.
** 
** @param pids : Array of pids of child processes
** @return [void]
** 
***************************************************************/
void roundRobinProcessExecution(pid_t * pids, int processCount) {

	int i;
	for ( i = 0; i < processCount; i++ ) {
		kill( pids[i], SIGUSR1 ); // Start process
		kill( pids[i], SIGSTOP ); // Immidiately stop
	}


	p_link *node = ( p_link * ) createCircularPidLinkList( pids, processCount );


	// Cycle through pids, remove p_links if the process has completed
	if ( processCount != 0 ) printf("\n--- BEGIN ---\n\n");
	while (processCount > 0) {

		// Look ahead since we have a one way p_linked list
		while ( isProcessComplete( node -> next -> pid ) && processCount > 0 ) {

			// Advance the next pointer to skip next process
			p_link *completedNode = node -> next;

			node -> next = node -> next -> next; 

			free( completedNode );

			processCount--;
		}

		if ( processCount > 0 ) {
			
			printf("--- STARTING %d ---\n", node -> pid); // Regain Control
			executeProcess( node -> pid, DEFAULT_TIME_SLICE ); // Executes process
			printf("--- STOPPING %d ---\n\n", node -> pid); // Regain Control

			node = node -> next;
		}

	}
	// All process complete after this point
}



/* Open Proc File **********************************************
** 
** Gets proc file pointer for specified pid.
** 
** @param pid_t pid : specified pid
** @return FILE * : file pointer of pid proc info
**
***************************************************************/
FILE *getProcFileForPid( pid_t pid, char * extension ) {

	FILE * procFile = NULL;
	
	char filename[ DEFAULT_BUFFER_SIZE ];
	snprintf( filename, DEFAULT_BUFFER_SIZE - 1, "/proc/%d/%s", pid, extension );

	procFile = fopen( filename, "r" );

	return procFile;
}



/* Get Proc File Data ******************************************
** 
** Pulls requested data from a processes proc file.
** 
** @param FILE * procFile : pointer to /proc/<pid>/status file
** @return char ** : our return values
**
***************************************************************/
void getProcStatusData( FILE * procFile ) {

	if ( !procFile ) perror("File error in getProcStatusData");

	size_t len 			= 128;
	char * line 		= NULL;

	
	// DATA TO PULL
	char * processName = NULL;
	char * State = NULL;
	char * VmPeak = NULL;
	char * VmSize = NULL;

	line = malloc( len * sizeof(char) );
	while( !processName || !State || !VmPeak || !VmSize ) {

		if( getline( &line, &len, procFile ) == -1 ) {
			if(!processName) {
				processName = malloc(11 * sizeof(char));
				strcpy(processName, "NOT FOUND.");
			}
			if(!State) {
				State = malloc(11 * sizeof(char));
				strcpy(State, "NOT FOUND.");
			}
			if(!VmPeak) {
				VmPeak = malloc(11 * sizeof(char));
				strcpy(VmPeak, "NOT FOUND.");
			}
			if(!VmSize) {
				VmSize = malloc(11 * sizeof(char));
				strcpy(VmSize, "NOT FOUND.");
			}
		}


		if( !strncmp( line, "Name:\t", 6 ) ) {
			processName = strdup( &line[6] );
			strtok( processName, "\n" );
		}
		else if( !strncmp( line, "State:\t", 7 ) ) {
			State = strdup( &line[7] );
			strtok( State, "\n" );
		}
		else if( !strncmp( line, "VmPeak:\t", 8 ) ) {
			VmPeak = strdup( &line[8] );
			strtok( VmPeak, "\n" );
		}
		else if( !strncmp( line, "VmSize:\t", 8 ) ) {
			VmSize = strdup( &line[8] );
			strtok( VmSize, "\n" );
		}

	}
	free( line );
	
	if ( processName ) {
		printf("# Name:   %s\n", processName);
		free( processName );
	}
	if ( State ) {
		printf("# State:  %s\n", State);
		free( State );
	}
	if ( VmPeak ) {
		printf("# VmPeak: %s\n", VmPeak);
		free( VmPeak );
	}
	if ( VmSize ) {
		printf("# VmSize: %s\n", VmSize);
		free( VmSize );
	}

	return;

}



/* Get Proc File Data ******************************************
** 
** Pulls requested data from a processes proc file.
** 
** @param FILE * procFile : pointer to /proc/<pid>/sched file
** @return char ** : our return values
**
***************************************************************/
void getProcSchedData( FILE * procFile ) {

	if ( !procFile ) perror("File error in getProcSchedData");

	size_t len 			= 128;
	char * line 		= NULL;

	line = malloc( len * sizeof(char) );
	
	// DATA TO PULL
	char * runtime     	= NULL;
	char * sub_runtime  = NULL;

	while( !runtime ) {

		if( getline( &line, &len, procFile ) == -1 ) {
			if ( !runtime ) runtime = "NOT FOUND.";
		}

		if( !strncmp( line, "se.vruntime", 11 ) ) {
			runtime = strdup( &line[11] );
			int start = 0;
			while (isspace(runtime[start]) || runtime[start] == ':') start++;
			sub_runtime = strdup( &runtime[start] );
			strtok( sub_runtime, "\n");
		}

	}
	free( line );

	len = strlen(runtime);
	runtime[len - 1] = 0;

	printf("# Runtime: %s nanoseconds\n", sub_runtime);

	if(sub_runtime) free( sub_runtime);
	if(runtime) 	free( runtime );

	return;
}




// 









// 9 lines 

/* Function short name *****************************************
** 
** Longer Description.
** 
** @param variableName : desc
** @param variabl2Name : desc
** @return : desc
**
***************************************************************/
