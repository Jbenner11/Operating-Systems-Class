#include <signal.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "p1fxns.h"
#include "usps.h"

#define DEFAULT_PROCS 25
#define DEFAULT_ARGS  128
#define BUFF_SIZE     2048

struct proc {
    int *pid;
    int *status;
    char *command_line;
    char **arguments;
    char *program;
}; 

void add_proc( int count, Proc *process, Proc **procs_array, int sprocs, int nprocs) {
    if ( sprocs < 0 ) {
        //allocate arroy of proc stucture with size default proc
        procs_array = (Proc**)malloc( DEFAULT_PROCS * sizeof( Proc * ) );
        sprocs = DEFAULT_PROCS;
    }
    else if ( sprocs <= nprocs ) {
        //reallocate array of proc structure by size of sprocs + DEFAULT PROCS
        sprocs += DEFAULT_PROCS;
        procs_array = (Proc**)realloc(procs_array, sprocs * sizeof ( Proc * ) );
    }
    //fill in procs[nprocs]
    procs_array[count] = process;
    nprocs++;
}


Proc *create_proc( ) {
    Proc* process = (Proc*)malloc( sizeof( Proc ) );
   
    process->pid          = NULL;
    process->status       = NULL;
    process->command_line = NULL;
    process->arguments    = NULL;
    process->program      = NULL;
    
    process->arguments = (char**)malloc( DEFAULT_ARGS * sizeof( char*) );

    return process;
}

void prepare_argument_structure( Proc *process) {
    char program[BUFF_SIZE], argument[BUFF_SIZE];
    char *line = process->command_line;

    //set programs and arguments arrays
    int start = 0;
    int i = 0;
   process->program = p1strdup( program );
    
    int next = p1getword( line, start, argument );
    while ( next != -1 ) {
        //printf( "arg %s\n", argument );
        process->arguments[i] = p1strdup( argument );
        
        start += next;
        next = p1getword( line, start, argument );
        i++;
    }
    process->program = process->arguments[0];
    printf("program: %s\n", process->program );
    printf("arguments: %s\n", process->arguments[0] );

}

//returns a char* with a word at each index of line, 
//while line[i] != NULL keep getting commands
char * get_line( ) {
    char *line = (char *)malloc( BUFF_SIZE * sizeof( char ) );
    int x = p1getline(0, line, BUFF_SIZE);

    if ( x == 0 ) 
        return NULL;
       
    line[x-1] = '\0';
    printf("x = %d\n", x);
    printf("line[x-1] = %s\n", line[x-1]);
    printf("line = %s\n", line);
    
    return line;
}


int main ( int argc, char* argv[] ) {
    /* Initialize some values at the start */
    // i and j used
    Proc **procs_array = NULL;
    int sprocs = -1;
    int nprocs =  0;
    Proc *process = NULL;
    
    int i;
    char *line = get_line();
    for (i = 0; line != NULL; i++ ) {
        process = create_proc();
        process->command_line = line;
        process->pid = fork(); 
        printf( "pid= %d\n", process->pid );
        if ( process->pid == 0 ) {
            prepare_argument_structure( process );
            // Party 2 working in here before execvp
            //sigwait
            
            execvp(process->program, process->arguments );
        }
        add_proc( i, process, procs_array, sprocs, nprocs );
        line = get_line();
    }

    int j;
    for ( j = 0; j <= nprocs; j++ ) {
        wait( process->pid );
    }

    pthread_t tid; /* thread identifier */
    pthread_attr_t attr; /* set of thread attributes */


    /* check only one file is inputed 
    if ( argc != 2 ) {
        printf( "usage error: should be ./<name of exec> file1" );
        exit(EXIT_FAILURE);
    }
    */
}