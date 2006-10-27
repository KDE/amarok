// Ovidiu Gheorghioiu <ovy@alum.mit.edu>, (C) 2006
// License: GNU General Public License V2
//
// Stress-test the AtomicString class for thread safety. Run on SMP for maximum exposure.
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "atomicstring.h"

void *
Worker(void *num) {
    srand( reinterpret_cast<int>( num ) );
    QString base = "str";
    // create 5 strings, destroy them, copy them around
    const int kNumStrings = 5;
    AtomicString *atStrings[kNumStrings * 2];
    for( int i = 0; i < kNumStrings * 2; i++ ) atStrings[i] = NULL;
    const int kIterations = 100000;
    for( int i = 0; i < kIterations; i++ ) {
	int k = rand() % (kNumStrings * 2);
	if( atStrings[k] == NULL ) {
	    // the upper half are sometimes copies of the corresponding
	    // lower half strings
	    if( k >= kNumStrings && atStrings[k % kNumStrings] != NULL ) {
		atStrings[k] = new AtomicString( *atStrings[k % kNumStrings] );
	    } else {
		atStrings[k] = new AtomicString( base + QString::number( k ) );
	    }
	} else {
	    // check the string; could be either upper or lower
	    QString str = atStrings[k]->string();
	    if( str != base + QString::number( k )
		&& str != base + QString::number( k % kNumStrings ) ) {
		qFatal( "unexpected atStrings[%d]: %s", k, str.ascii() );
	    }
	    delete atStrings[k];
	    atStrings[k] = NULL;
	}
    }

    return NULL;
}

int main() {
    const int kWorkers = 2;

    pthread_t workers[kWorkers];

    for( int i = 0; i < kWorkers; i++ ) {
	if( pthread_create(& workers[i], NULL,
			   & Worker,
			   reinterpret_cast<void *>(i))
	    != 0)
	    qFatal( "Could not create thread %d", i );
    }

    for( int i = 0; i < kWorkers; i++ ) {
	void *thread_return;
	pthread_join( workers[i], &thread_return );
    }
}
