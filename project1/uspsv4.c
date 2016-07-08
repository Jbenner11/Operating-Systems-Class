#include <signal.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "p1fxns.h"
#include "usps.h"

#define DEFAULT_ARGS         30
#define BUFF_SIZE            2048



struct node {
    Node *next;
    Proc *process;
};

struct linkedList {
    Node *head;
};


struct proc {
    int *pid;
    char *command_line;
    char **arguments;
    char *program;
}; 

double DEFAULT_EXEC_TIME = 1;
int sig_received = 0;

LinkedList *create_list ( ) {
    LinkedList *ll = (LinkedList*)malloc( sizeof( LinkedList ) );
    ll->head = NULL;
    return ll;
}


void add_node ( LinkedList *list, Proc *process) {
    if (list == NULL ) {
        p1perror( 1, "ERROR LIST IS NULL" );
        return;
    }

    Node *newNode = (Node*)malloc( sizeof( Node ) );
    newNode->process = process;
    
    
    newNode->next = NULL;

    if (list->head == NULL ) {
        list->head = newNode;
        return;
    }

    Node *temp = list->head;
    while( temp->next != NULL ) {
        temp = temp->next;
    }
    temp->next = newNode;
}


Node *get_node( LinkedList *list, int count ) {
    int i;
    Node *temp = list->head;
    for ( i = 0; i < count; i++ ) {
        if ( temp->next == NULL ) {
            p1perror( 1, " NEXT SHOULD NOT BE NULL" );
            return;
        }
        temp = temp->next;
    }
    return temp;
}


int list_length( LinkedList *list ) {
    if (list == NULL ) {
        p1perror( 1, "LENGTH: LIST SHOULD NOT BE NULL" );
        return 0;
    }
    int count = 0;
    Node *temp = list->head;

    while ( temp->next != NULL ) {
        count ++;
        temp = temp->next;
    }
    return count;
}


Proc *create_proc( ) {
    Proc* process = (Proc*)malloc( sizeof( Proc ) );
   
    process->pid          = NULL;
    process->command_line = NULL;
    process->arguments    = NULL;
    process->program      = NULL;
    
    process->arguments = (char**)malloc( DEFAULT_ARGS * sizeof( char*) );
    int i;
    for ( i = 0; i < DEFAULT_ARGS; i++ )
        process->arguments[i] = NULL;

    return process;
}


void prepare_argument_structure( Proc *process) {
    char argument[BUFF_SIZE];
    int i;
    for( i = 0; i < BUFF_SIZE; i++ )
        argument[i] = '\0';
    char *line = process->command_line;

    //set programs and arguments arrays
    int start = 0;
    i = 0;
    //process->program = p1strdup( program );
    
    int next = p1getword( line, start, argument );
    while ( next != -1 ) {
        process->arguments[i] = p1strdup( argument );
        
        start = next;
        int j;
        for( j = 0; j < BUFF_SIZE; j++ )
            argument[i] = '\0';
        next = p1getword( line, start, argument );
        i++;
    }
    process->program = process->arguments[0];
    //print_process_overview( process, "Parent" );
}

//returns a char* with a word at each index of line, 
//while line[i] != NULL keep getting commands
char * get_line( ) {
    char *line = (char *)malloc( BUFF_SIZE * sizeof( char ) );
    int x = p1getline(0, line, BUFF_SIZE);

    if ( x == 0 ) {
        free ( line );
        return NULL;
    }
    line[x-1] = '\0';
    
    return line;
}


void destroy_line( char *line ) {
    free ( line );
    line = NULL;
}

void destroy_single_node ( Node *node ) { // We use this to delete the next node in RoundRobin, careful with this destroy

    if ( node->process != NULL ) {
        destroy_process ( node->process );
        node->process == NULL;
    }
    
    node->next = NULL;

    free ( node );
    node = NULL;
}


void destroy_list ( LinkedList *list ) {
    if ( list->head != NULL) {
            destroy_single_node( list->head );
    }
    free ( list );
    list = NULL;      
}


void destroy_process ( Proc *proc ) {
    int i;
    if ( proc != NULL ) {
        if ( proc->pid != NULL ) {
            proc->pid = NULL;
        }

        if ( proc->program != NULL ) {
            proc->program = NULL;
        }

        if ( proc->arguments != NULL ) {
            // free all pointer in arguments
            for ( i = 0; i < DEFAULT_ARGS ; i++ ) { 
                if (proc->arguments[i] != NULL )
                    free( proc->arguments[i] );
            }
            free( proc->arguments );
            proc->arguments = NULL;
        }

        if ( proc->command_line != NULL ) {
            proc->command_line = NULL;
        }
    }
    free( proc );
    proc = NULL;
}


void sig_alarm_handler( int signo ) {
    // reset alarm
        signal( SIGALRM, sig_alarm_handler );
}


void sig_handler( int signo ) {
    if ( signo == SIGUSR1) {
        sig_received++;
    }
}

int is_process_completed( Proc *process ) {
    int childStatus, status;

    childStatus = waitpid( process->pid, &status, WNOHANG );
    if ( childStatus == 0 ) 
        return 0;
    else
        return 1;
}


void execute_child_process ( Proc *process, sigset_t signal_set, int sig_ret ) {
    int test;

    test = sigwait( &signal_set, &sig_ret );

    
    execvp(process->program, process->arguments );

    exit( 0 );
}


void create_circular_list ( LinkedList *list ) {
    Node *temp = list->head;
    
    while( temp->next != NULL ) {
        temp = temp->next;
    } // temp is now last node
    
    temp->next = list->head; // connect link making a circle

}


void round_robin_scheduling( LinkedList *list, int processCount )  {
    int j;
    //cycle through and start processes immediately then stop them for round robin scheduling
    for ( j = 0; j <= list_length( list ); j++ ) {
        Node *killNode = get_node( list, j );
        kill ( killNode->process->pid, SIGUSR1 );
        //kill ( killNode->process->pid, SIGSTOP ); // do we want to stop them right after or after all are running
    }
    int k; 
    for ( k = 0; k <= list_length( list ); k++ ) {
        Node *killNode2 = get_node( list, k );
        kill ( killNode2->process->pid, SIGSTOP ); // do we want to stop them right after or after all are running
    }
    
    create_circular_list ( list );

    if ( processCount != 0 ) {
        p1putstr( 1, "------begin round robin-------\n\n" );
    }

    Node *currNode = list->head;
    
    
    // while there are still have processes running
    while ( processCount > 0 ) {
        // look ahead to see if next process is completed
        
        while ( is_process_completed( currNode->next->process  ) != 0 && processCount > 0 ) { // Not running
            Node *finishedNode = currNode->next;
            
            //print_process_overview( finishedNode->process, "Round Robin" );

            currNode->next = finishedNode-> next;

            if ( finishedNode->process->pid == list->head->process->pid );
                list->head = list->head->next;

            
            
            destroy_single_node( finishedNode ); //free finished process

            processCount--; // decrease processCount
            
            p1putstr( 1, "\nProcessCount " );
            p1putint( 1, processCount );
        }
      
     
        if ( processCount > 0 ) {
            p1putstr( 1, "-----Starting Process " );
            p1putint( 1, currNode->process->pid );
            p1putstr( 1, "----\n" );
            
            execute_process( currNode->process, DEFAULT_EXEC_TIME );
        
            p1putstr( 1, "\n-----Ending  Process------ \n\n" );
         
            // if at end of list reset
            currNode = currNode->next;
        }
    }
}


int string_compare( char *str1, char *str2, int max ) {
    int flag = 0, i = 0, j = 0;
    
    for ( i = 0; i < max; i++ ) {
        if ( str1[i] != '\0' && str2[i] != '\0' ) {
            if ( str1[i] != str2[i] )
                return 0;

        }
    }

        return 1;
}



string_concat( char *dest, char *src ) {
    int i = 0, j = 0;
    
    while ( dest[i] != '\0' )
        i++;
      
    while( src[j] != '\0' ) {
        dest[ i ] = src[ j ];
        j++;
        i++;
    }   
    dest[i] = '\0';
}


/** Reverse a String
 *  
 *  returns: char* pointer to string if successful
 *           NULL otherwise
 */
char *reverse_string( char *str ) {

    if ( str == NULL ) {
        p1perror( 1, "A null string was passed to reversed_string\n" );
        return NULL;
    }

    int str_len = p1strlen( str );

    /*
     * FREE ME! From the heap.
     * The mallow needs to be big enough for the string and the null character so, str_len + 1
     */
    char *reversed_string = malloc( str_len * sizeof( char ) );

    int i;
    for ( i = 0; i < str_len; i++ )
        reversed_string[ i ] = str[ str_len - i - 1 ];

    reversed_string[ str_len ] = '\0'; // 'cap' the end of the string with the EOS character
    
    return reversed_string;
}


char *my_itoa( int num ) {
    char *PidString = malloc( BUFF_SIZE * sizeof( char ) );
    char *revPidString;
    int i = 0;
    if ( num < 0 )
        p1perror( 1, "Number conversion is negative, please use a positive number" );

    while ( num != 0 ) {
        int rem = num % 10;
        if (rem > 9)
            PidString[i++] = (rem-10)+ 'a';
        else
            PidString[i++] = rem + '0';
        num = num/10;
    }
    PidString[i] = '\0';
    
    revPidString = reverse_string( PidString );
    free ( PidString );
    return revPidString;
}

// result needs to be freed at some point
FILE *get_proc_file_pid ( Proc *process, char *extension ) { // extension is folder name such as fdinfo or sched
    FILE *procFile = NULL;
    int PidInt = process->pid;
    char *PidStr;
    
    char tfilename[ BUFF_SIZE ] = "/proc/";//    /proc/
    
    
    // This string is on the STACK
    char *str = "This is it and there is more to this string";

    // This returned pointer is to the HEAP, be sure to free it
    char *reversed_str = reverse_string( str );

    PidStr = my_itoa( PidInt ); // need to free this

    string_concat( tfilename, PidStr ); //       /proc/pid
    string_concat( tfilename, "/" ); //          /proc/pid/
    string_concat( tfilename, extension ); //     /proc/pid/entension

    char *finFilename = p1strdup ( tfilename );

    procFile = fopen( finFilename, "r" );
   
    free( reversed_str );
    free( finFilename );
    free( PidStr );   

    return procFile;
}


void clear_newline( char *line ) {
    int len, i;

    len = p1strlen( line );
    for ( i = 0; i < len; i++ ) {
        if( line[i] == '\n' )
            line[i] = ' ';
    }
}


void get_proc_status_data ( FILE * procFile ) {
    if ( !procFile ) 
        p1perror ( 1, "Error in function get_proc_status_data" );
   
    size_t length = 256;
    char *line = NULL;

    char *processName    = NULL;
    char *state          = NULL;
    char *VmPeak         = NULL;
    char *VmSize         = NULL;

    line = malloc( length * sizeof( char ) );
    while ( !processName || !state || !VmPeak || ! VmSize ) {
        if ( getline( &line, &length, procFile ) == -1 ) {
            if ( processName == NULL ) 
                processName = p1strdup ("NAME NOT FOUND" );

            if ( state == NULL ) 
                state = p1strdup ("STATE NOT FOUND" );
            
            if ( VmPeak == NULL ) 
                VmPeak = p1strdup ("VmPEAK NOT FOUND" );

            if ( VmSize == NULL ) 
                VmSize = p1strdup ("VmSIZE NOT FOUND" );
        }
        
        if ( string_compare ( line, "Name:\t", 6 ) == 1 ) {
            processName = p1strdup( &line[6] );
            clear_newline( processName );
        }
    
        if ( string_compare ( line, "State:\t", 7 ) == 1 ) {
            state = p1strdup( &line[7] );
            clear_newline( state );
        }

        if ( string_compare ( line, "VmPeak:\t", 8 ) == 1 ) {
            VmPeak = p1strdup( &line[8] );
            clear_newline( VmPeak );
        }

        if ( string_compare ( line, "VmSize:\t", 8 ) == 1 ) {
            VmSize = p1strdup( &line[8] );
            clear_newline( VmSize );
        }
    }

    free ( line );

    if ( processName != NULL ) {
        p1putstr( 1, "Name: " );
        p1putstr( 1, processName );
        free ( processName );
    }

    if ( state != NULL ) {
        p1putstr( 1, "\nstate: " );
        p1putstr( 1, state );
        free ( state );
    }

    if ( VmPeak !=  NULL ) {
        p1putstr( 1, "\nVmPeak: " );
        p1putstr( 1, VmPeak );
        free ( VmPeak );
    }
    
    if ( VmSize != NULL ) {
        p1putstr( 1, "\nVmSize: " );
        p1putstr( 1, VmSize );
        free ( VmSize );
    }

    return;
}


void get_proc_schedule_data ( FILE * procFile ) {
    if ( !procFile ) 
        p1perror ( 1, "Error in function get_proc_sched_data" );
   
    size_t length = 256;
    char *line = NULL;

    char *runtime          = NULL;
    char *runtime2         = NULL;
    line = (char *)malloc( length * sizeof( char ) );

    while ( runtime == NULL ) {
        if ( getline ( &line, &length, procFile ) == -1 ) {
            if ( runtime == NULL )
                runtime = "RUNTIME NOT FOUND.";
        }
        if ( string_compare( line, "se.vruntime", 11 ) == 1 ) {
            runtime = p1strdup( &line[11] );
            int begin = 0;
            while ( isspace( runtime[begin] ) || runtime[begin] == ":"  )
                begin++;
            
            runtime2 = p1strdup( &runtime[begin] );
            clear_newline( runtime2 );
        }
    }    

    free( line );

    length = p1strlen( runtime );
    runtime[length -1] = 0;

    p1putstr( 1, "\nRuntime is: ");
    p1putstr( 1, runtime2);
    p1putstr( 1, "in nanoseconds");

    if ( runtime != NULL)
        free( runtime );
    if (runtime2 != NULL )
        free( runtime2 );

    return;
}


void execute_process( Proc * process, double sleepTime  ) {
    kill( process->pid, SIGCONT ); //continue process
    signal( SIGALRM, sig_alarm_handler )   ;
   
    p1putstr( 1, "Process Information\n" );
    FILE * statusFile = NULL;
    statusFile = get_proc_file_pid ( process, "status" );
    get_proc_status_data( statusFile );
    fclose( statusFile );


    FILE * scheduleFile = NULL;
    scheduleFile = get_proc_file_pid ( process, "sched" );
    get_proc_schedule_data( scheduleFile );
    fclose( scheduleFile );
   
    p1putstr( 1, "\nEnd Process Information\n" ); 

    if ( sleepTime < 1 ) 
        ualarm( ( int )(sleepTime * 1000000) % 1000000, 0 ); // if time is less than 1 use modulo to find how long to sleep
    else
        alarm( sleepTime ); // else set alarm for sleepTime amount of seconds

    //p1putstr( 1, "Pausing!\n" );
    pause(); //pause until process is stopped

    kill( process->pid, SIGSTOP ); // stop process
    //p1putstr( 1, "Killing it!\n" );
}


int main ( int argc, char* argv[] ) {
    /* Initialize some values at the start   | i and j used */
    LinkedList *LList;
    Proc *process = NULL;
    int i;
    int childProcessPid;
    LList = create_list();  // create linked list
    sigset_t signal_set;
    int sig_ret;
    
    sigemptyset( &signal_set);
    sigaddset( &signal_set, SIGUSR1 );
    sigprocmask( SIG_BLOCK, &signal_set, NULL);
    
    if( signal( SIGUSR1, sig_handler ) == SIG_ERR ) {
        p1putstr(1, "can't catch SIGUSR1" );
        return 1;
    }
    
    char *line = get_line();
    for (i = 0; line != NULL; i++ ) {
        process = create_proc();
        process->command_line = line;
        prepare_argument_structure( process ); // used to be right above execute child process8
        process->pid = fork();  
        if ( process->pid == 0 ) {
            // child process starts here
            sig_handler( SIGUSR1 ); 
            while ( ! sig_received )  //will be zero until sig_handler is run
                sleep(1);
            //prepare_argument_structure( process );
    
            //print_process_overview( process, "Child" ); // prints process attrs

            signal( SIGUSR1, sig_handler );
            execute_child_process( process, signal_set, sig_ret ); // stuck in here, process not being created correctly
            // child process ends here
        }
        
        else if ( process->pid < 0 ) {
            perror( "fork" );
            p1putstr( 1, "ERROR: BAD PID" );
            return;
        }
        
        //parent continue
        
        
        add_node( LList, process );

        destroy_line( line ); 
        
        line = get_line(); //Parents are fine, they have all the requirements for addNode
        
    }
    // send Signal to each child process to continue
    sleep(1);
    
    round_robin_scheduling( LList, list_length( LList ) ); //Nodes are fine going into RRS

    p1putstr( 1, "\nRound Robin Finished \n" );
    
    destroy_line ( line );
    destroy_list ( LList );
    p1putstr( 1, "End of Code\n" );

}
    
