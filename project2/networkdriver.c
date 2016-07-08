/*
 * Author: Joel
 * Edited: 2016-05-12
 *
 */

#include "networkdriver.h"
#include "packetdescriptor.h"
#include "packetdescriptorcreator.h"
#include "freepacketdescriptorstore.h"
#include "freepacketdescriptorstore__full.h"
#include "networkdevice.h"
#include "BoundedBuffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* GLOBALS */


FreePacketDescriptorStore *fpds;
BoundedBuffer *toAppBuffer[MAX_PID + 1];
BoundedBuffer *toDeviceBuffer;


pthread_mutex_t to_app_mutex    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t to_device_mutex = PTHREAD_MUTEX_INITIALIZER;

int BUFF_PACKET_LIMIT 			  = 20;

/* Helper Functions */


/** Blocking Send Packet
 *
 * " These calls hand in a PacketDescriptor for dispatching
 *   ...
 *   The blocking call will usually return promptly, but there may be
 *   a delay while it waits for space in your buffers.
 *   Neither call should delay until the packet is actually sent "
 *
 * @param  pd [packetDesctriptor to be sent]
 */
void blocking_send_packet( PacketDescriptor *pd ) {
	return (blockingWriteBB( toDeviceBuffer, (void *)pd ) ); 	/* write to buffer  */ 
}


/** Non-Blocking Send Packet
 *
 * " These calls hand in a PacketDescriptor for dispatching
 *   The nonblocking call must return promptly, indicating whether or
 *   not the indicated packet has been accepted by your code
 *   ( it might not be if your internal buffer is ful ) 1=OK, 0=not OK
 *   ...
 *   Neither call should delay until the packet is actually sent "
 *
 * @param  pd [packetDescriptor to be sent]
 * @return    1 if OK
 *            0 if not OK
 */
int nonblocking_send_packet( PacketDescriptor *pd ) {
   	return ( nonblockingWriteBB( toDeviceBuffer, (void *)pd ) );
}


/** Blocking Get Packet
 *
 * " These represent requests for packets by the application threads
 *   ...
 *   The blocking call only returns when a packet has been received
 *   for the indicated process, and the first arg points at it.
 *   Both calls indicate their process number and should only be
 *   given appropriate packets. You may use a small bounded buffer
 *   to hold packets that haven't yet been collected by a process,
 *   but are also allowed to discard extra packets if at least one
 *   is waiting uncollected for the same PID. i.e. applications must
 *   collect their packets reasonably promptly, or risk packet loss. "
 *
 * @param pd  [description]
 * @param PID [description]
 */
void blocking_get_packet( PacketDescriptor **pd, PID pid ) {
		*pd = blockingReadBB( toAppBuffer[pid] );		/* set pd to next packet  */
}


/** Non-Blocking Get Packet
 *
 * " These represent requests for packets by the application threads
 *   The nonblocking call must return pr/omptly, with the result 1 if
 *   a packet was found ( and the first argument set accordingly ) or
 *   0 if no packet was waiting.
 *   ...
 *   Both calls indicate their process number and should only be
 *   given appropriate packets. You may use a small bounded buffer
 *   to hold packets that haven't yet been collected by a process,
 *   but are also allowed to discard extra packets if at least one
 *   is waiting uncollected for the same PID. i.e. applications must
 *   collect their packets reasonably promptly, or risk packet loss. "
 *
 * @param  pd  [The PacketDescriptor that we want to find in our buffer]
 * @param  PID [pid of desired packet]
 * @return     1 if a packet was found ( and the first argument set accordingly )
 *             0 if no packet was waiting
 */
int nonblocking_get_packet( PacketDescriptor **pd, PID pid ) {
		return (nonblockingReadBB( toAppBuffer[pid], (void **)pd ) );	/* if packet is in buffer */
}

/**  Sending Thread Funtion
 * " This function is run when we initialize the networkdriver.
 *   it is responsible for pulling packets out of the to_device 
 *   buffer and sending them to the device using the 
 *   send_packet( *nd, *pd ) function. Upon success the packet 
 *   should be returned to the store. If the send is unsuccessful 
 *   we try 2 more times and if it fails again the packet is lost
 *   and we starting trying to send the next. This function should 
 *   be running as long as the packets are getting send (i.e. as 
 *   long as the OS running, it will not end voluntarily).

 * @param  *nd  [The networkDevice we are sending packets too]
 * returns a (void *) but we use NULL because it should never return 
 */
void *thread_send( void *nd ) {
	while ( 1 ) {
		PacketDescriptor *pd;
		if ( !nonblockingReadBB( toDeviceBuffer,(void **)&pd ) ) {  /* get item from queue we want */
			continue;
		}

		int i;
		pthread_mutex_lock( &to_device_mutex );				/* Do i need this? */
		for( i = 0; i < 3; i++ ) {

			if( send_packet( nd, pd ) )  	/* send packet */
				blocking_put_pd( fpds, pd ); /* put back in descriptor store if successful, otherwise ditch the packet */
		}
		pthread_mutex_unlock( &to_device_mutex );				/* Do i need this? */
	}
	return (void *) NULL; /* Never get here, This is how it is supposed to work */
}


/**  Receiving Thread Funtion
 * " This function is run when we initialize the networkdriver.
 *   It is responsible for getting packets from the 
 *   freePacketDesciptorStore, initializing it, registering it, and
 *   waiting until it is filled by the networkDevice. 
 *   This function should, then the function puts it in the to_app_buffer
 *   where it can be used later. This function will be running as long as
     the packets are getting filled (i.e. as 
 *   long as the OS is running, it will not end voluntarily).

 * @param  *nd  [The network device that we are registering packets with]
 * returns a (void *) but we use NULL because it should never return 
*/
void *thread_receive( void *nd ) { /* What does this function need? */
	while( 1 ) {
		PacketDescriptor *pd;							/* initialize  */
	    	
		if ( nonblocking_get_pd( fpds, &pd ) ) {				/* needs double pointer?? get pd and put in pd */ 
			init_packet_descriptor( pd );						/* initialize packet */
			register_receiving_packetdescriptor( nd, pd  );		/* register Packet */
			await_incoming_packet( nd );						/* wait to fill packet ptr */
		
		/* I feel like there should be more here */
			int pid = packet_descriptor_get_pid( pd );
			blockingWriteBB( toAppBuffer[pid], (void *)pd );
		}
	}
	return (void *) NULL; /* Never get here, This is how it is supposed to work */
}

/** Init Network Driver
 
 * " Called before any other methods, to allow you to initialise
 *   data structures and start any internal threads.
 *
 *   Hint: just divide the memory up into pieces of the right size
 *   			 passing in pointers to each of them "
 *
 * @param nd         The NetworkDevice that you must drive
 * @param mem_start  Memory location for PacketDescriptors
 * @param mem_length Memory size
 * @param fpds_ptr   You hand back a FreePacketDescriptorStore into
 *   				     which you have put the divided up memory
 */
void init_network_driver( NetworkDevice *nd, void *mem_start, unsigned long mem_length,
                          FreePacketDescriptorStore **fpds_ptr ) {
	
	/* First we are creating the Free Packet Descritor Store
     Setup new_fpds memory space */
    fpds = create_fpds( );
	
    /* load fpds with packet descriptors comstructed from mem start and mem length */
    int numberOfPackets = create_free_packet_descriptors( fpds, mem_start, mem_length);
	
	/* Create any buffers required by threads | May need one more */
	int i;
	for ( i = 0; i < MAX_PID + 1; i ++ ) {
		toAppBuffer[i] = createBB( BUFF_PACKET_LIMIT );
	}
	toDeviceBuffer = createBB( BUFF_PACKET_LIMIT );
    
    /* Create any threads required for implementation 
	 ( One per system, send and receive ). */
    pthread_t send_thread;
    pthread_t receive_thread;
    
    pthread_create( &receive_thread, NULL, thread_send   , (void *)nd );	
    pthread_create( &send_thread   , NULL, thread_receive, (void *)nd );	
	
	/* return values and clean up  */
    *fpds_ptr = fpds;
	return;
}

