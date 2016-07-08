/*
Name:   Joel Benner
DuckID: jbenner
Project_0
*/

#include "date.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct date {
	int day;
	int month;
	int year;
};

void parseDate( char *str, char garbage) {
	char *src, *dst;
	for (src = dst = str; *src != '\0'; src++) {
		*dst = *src;
		if (*dst != garbage) dst++;
	}
	*dst = '\0';
}	


Date *date_create( char *datestr ) {
	char day[3]; day[2] = '\0';
	char month[3]; month[2] = '\0';
	char year[5]; year[4] = '\0';
	int d, m, y;
	char dateString[11];

	strcpy(dateString, datestr); /* confirmed to work */

	Date *newDate = (Date *)malloc(sizeof(Date));
	parseDate(dateString, '/');
	strncpy(day, &dateString[0], 2);
	strncpy(month, &dateString[2], 2);
	strncpy(year, &dateString[4], 4);
	
	d = atoi(day);
	m = atoi(month);
	y = atoi(year);

	
	if ( newDate == NULL ) 
		return NULL;

	newDate->day       = d;
	newDate->month     = m;
	newDate->year  	   = y;

	return (newDate);



}

Date *date_duplicate( Date *d ) {
	if (d == NULL) 
		return NULL;

	Date *dup = ( Date * )malloc( sizeof( Date ) );
	*dup = *d; 	
	return dup;
}


int date_compare( Date *date1, Date *date2 ) { 
	if (date1->year < date2->year) 
		return -1;

	else if (date1->year == date2->year) {
		if (date1->month < date2->month) 
			return -1;

		else if (date1->month == date2->month) {
			if (date1->day < date2->day) {
				return -1;
			}
			else if (date1->day == date2->day) 
				return 0;
			else 
				return 1;
		}
		else 
			return 1;
	}
	else 
		return 1;
}



void date_destroy( Date *d ) {
	if ( d != NULL )
        free( d );
    else
        printf( "Found null when destroying date\n" );
}
