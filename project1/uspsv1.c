#include <signal.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
    //printf("program: %s\n", process->program );
    //printf("arguments: %s\n", process->arguments[0] );
}

//returns a char* with a word at each index of line, 
//while line[i] != NULL keep getting commands
char * get_line( ) {
    char *line = (char *)malloc( BUFF_SIZE * sizeof( char ) );
    int x = p1getline(0, line, BUFF_SIZE);

    if ( x == 0 ) 
        return NULL;
       
    line[x-1] = '\0';
    
    return line;
}


void set_signal_block_list( sigset_t sig_set ) {
    sigemptyset( &sig_set);
    sigaddset( &sig_set, SIGUSR1 );
    sigprocmask( SIG_BLOCK, &sig_set, NULL);

}


void sig_handler( int signo, int sig_received ) {
    if ( signo == SIGUSR1)
        sig_received++;
}

int main ( int argc, char* argv[] ) {
    /* Initialize some values at the start */
    // i and j used
    Proc **procs_array = NULL;
    int sprocs = -1;
    int nprocs =  0;
    Proc *process = NULL;
    int i;
    //signal variables
    sigset_t signal_set;
    int sig_received = 0;
    int sig_ret;

    set_signal_block_list( signal_set ); // set up signal_set to block SIGUSR1
    
    if( signal( SIGUSR1, sig_handler ) == SIG_ERR ) {
        p1putstr(1, "can't catch SIGUSR1" );
        return 1;
    }

    char *line = get_line();
    for (i = 0; line != NULL; i++ ) {
        process = create_proc();
        process->command_line = line;
        process->pid = fork(); 
        printf( "pid= %d\n", process->pid );
        //parent skip
        if ( process->pid == 0 ) {
            // child process starts here
            //while ( ! sig_received )  //will be zero until sig_handler is run
                //sleep(1);

            prepare_argument_structure( process );
            
            //rest of child code including sigwait...i think
            int sigTest = sigwait( &signal_set, &sig_ret ); //freezing here ( GOOD )
            printf( "Child process %d has a sigtest of %d\n ", process->pid, sigTest);
            execvp(process->program, process->arguments );
            // child process ends here
        }
        //parent and fork failure branch -- what is this
        //parent continue
        add_proc( i, process, procs_array, sprocs, nprocs );
        line = get_line();
        
        // dont think stuff below goes here
        
        raise( SIGUSR1 );// maybe
        sigprocmask( SIG_UNBLOCK, &signal_set, NULL ); //maybe
    }
    //send Signal to each child process to continue
    printf( "made it to raise\n" );
    raise( SIGUSR1 );// maybe
    //printf( "Post raise/ pre sigprocmask\n" );
    sigprocmask( SIG_UNBLOCK, &signal_set, NULL ); //maybe
    //printf( "Post raise/ pre sigprocmask\n" );
    printf( "Post sigprocmask\n" );

    int j;
    for ( j = 0; j <= nprocs; j++ ) {
        wait( process->pid );
    }

    /* check only one file is inputed 
    if ( argc != 2 ) {
        printf( "usage error: should be ./<name of exec> file1" );
        exit(EXIT_FAILURE);
    }
    */
}
