/*
Name:   Joel Benner
DuckID: jbenner
Project_0
*/


#include "tldlist.h"
#include "date.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

struct tldlist {
	long count;
	Date *beginDate;
	Date *endDate;
	TLDNode *root;
};

struct tldnode {
	char *hostname;
	long count;
	TLDNode *left;
	TLDNode *right;
	TLDNode *parent;
};

struct tlditerator {
	TLDNode *next;
};


/* Helper Functions */

void recurseTree( TLDNode *n ) {
	if( n == NULL )
		return;
	printf( "%s\n", n->hostname );
	if( n->left != NULL )
		recurseTree( n->left );
	if( n->right != NULL )
		recurseTree( n->right );
}
		

void printTree( TLDList *t ) {
	printf( "These are the hostnames in your tree:\n\n" );
	recurseTree( t->root );
	printf( "\nDone printing\n" );
}

TLDNode *tldnode_create( char *hostname ) {
	TLDNode *n = malloc( sizeof( TLDNode ) );
	n->left 	= NULL;
	n->right 	= NULL;
	n->parent 	= NULL;
	int hostname_len = strlen( hostname );
	n->hostname 	= malloc( hostname_len + 1 );
	strcpy( n->hostname, hostname );
	n->hostname[ hostname_len ] = '\0';
	
	n->count 	= 1;
	return n;
}

TLDNode *tldnode_create_with_parent( char *hostname, TLDNode *parent ) {
	TLDNode *n = tldnode_create( hostname );
	n->parent = parent;
	return n;
}



void treeBuilder ( TLDNode *node, char *hostname ) {

	int strcmp_result = strcmp( hostname, node->hostname );

	if ( strcmp_result < 0 ) {
		if ( node->left != NULL ) {
			treeBuilder( node->left, hostname );
		}
		else {
			node->left = tldnode_create_with_parent( hostname, node );
		}
	}
	else if ( strcmp_result == 0 ) {
		node->count++;
	}
	else {
		if ( node->right != NULL ) {
			treeBuilder( node->right, hostname );
		}
		else {
			node->right = tldnode_create_with_parent( hostname, node );
		}	
	}
}


void tldnode_destroy ( TLDNode *node ) {
	if ( node != NULL ) {
		if ( node->left != NULL ) {
			tldnode_destroy ( node->left );
			node->left = NULL;
		}	
		if (node->right != NULL ) {
			tldnode_destroy ( node->right );
			node->right = NULL;
		}
		if ( node->hostname != NULL ) {
			free ( node->hostname );
			node->hostname = NULL;	
		}
        node->parent = NULL;
		free ( node );
	}
}

char *parseHostname ( char *hostname ) {
	char *tmpHostname = hostname;
	char *last = NULL;
	hostname = strtok( tmpHostname, "." );
	while ( tmpHostname != NULL ) {
		last = tmpHostname;
		tmpHostname = strtok( NULL, "." );
	}
	return ( last );
}

/* End Helper Functions */

/* NEEDS WORK */
TLDList *tldlist_create( Date *begin, Date *end ) {
	TLDList *newTLDList = ( TLDList * )malloc( sizeof( TLDList ) );
    
    Date *newBegin = date_duplicate ( begin );
    Date *newEnd = date_duplicate ( end );
    
	if ( newTLDList == NULL )
		return NULL;

	newTLDList->beginDate 	= newBegin;
	newTLDList->endDate   	= newEnd;
	newTLDList->count 	    = 0;	
	newTLDList->root 	    = NULL;
		
	return ( newTLDList );
}


void tldlist_destroy( TLDList *tld ) {
	if ( tld != NULL ) {
		if (tld->root != NULL) {
			tldnode_destroy ( tld->root );
			tld->root = NULL;
		}
        if ( tld->beginDate != NULL ) {
            date_destroy ( tld->beginDate );
            tld->beginDate = NULL;
        }
        if ( tld->endDate != NULL ) {
            date_destroy ( tld->endDate );
            tld->endDate = NULL;
        }
		free( tld );
	}
}


/* NEEDS WORK */
int tldlist_add( TLDList *tld, char *hostname, Date *d ) {	

	if ( date_compare( tld->beginDate, d) != 1 && date_compare ( tld->endDate, d) != -1 ) {
		/*create node, add to tree, update stuff */
		if ( tld->root == NULL ) {
			TLDNode *TLDRoot  = tldnode_create( parseHostname( hostname ) );
			tld->root = TLDRoot;
		}
		else {
			treeBuilder( tld->root, parseHostname( hostname ) );	
		}
		
		tld->count++;	
		return 1;
	}
	return 0;
}

long tldlist_count( TLDList *tld ) {
	return ( tld->count );
}


TLDNode *leftmostNode( TLDNode *node ) {
	while( node->left != NULL )
		node = node->left;
	return node;
}


TLDIterator *tldlist_iter_create( TLDList *tld ) {
	TLDIterator *newTLDIter = ( TLDIterator * )malloc( sizeof( TLDIterator )  );

	if ( newTLDIter == NULL )
		return NULL;	
	
	newTLDIter->next  = leftmostNode( tld->root );
	return ( newTLDIter );
}


TLDNode *getNextNode ( TLDIterator *iter ) {   /* node is iter->next */
	TLDNode *r = iter->next;

	if ( iter->next->right != NULL ) {
		iter->next = iter->next->right;
		iter->next = leftmostNode( iter->next );
		return ( r );
	}
	else {
		while ( true ) {
			if ( iter->next->parent == NULL ) {
				iter->next = NULL;
				return ( r );
			}	
			if ( iter->next->parent->left == iter->next ) {
				iter->next = iter->next->parent;
				return ( r );
			}
		iter->next = iter->next->parent;	
		}
	}
}

TLDNode *tldlist_iter_next( TLDIterator *iter ) {

	if ( iter->next == NULL )
		return NULL;

	return ( getNextNode( iter ) );	

}

void tldlist_iter_destroy( TLDIterator *iter) {
	if ( iter != NULL ) {
		if ( iter->next != NULL ) { 
			tldnode_destroy ( iter->next );
            iter->next = NULL;
		}
        free ( iter );
	}
}


char *tldnode_tldname( TLDNode *node ) {
	return ( node->hostname );
}


long tldnode_count( TLDNode *node ) {
	return (node->count);	
}

