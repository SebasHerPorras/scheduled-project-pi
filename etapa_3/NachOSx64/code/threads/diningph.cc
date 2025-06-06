#include "diningph.h"

DiningPh::DiningPh() {

    dp = new Lock( (char*)"dp" );

    for ( int i = 0; i < 5; i++ ) {
        self[ i ] = new Condition( "philo" );
        state[ i ] = Thinking;
    }

}


DiningPh::~DiningPh() {
}


void DiningPh::pickup( long who ) {

    dp->Acquire();

    state[ who ] = Hungry;
    test( who );
    if ( state[ who ] == Hungry )
        self[ who ]->Wait( dp );

    dp->Release();

}


void DiningPh::putdown( long who ) {

    dp->Acquire();

    state[ who ] = Thinking;
    test( (who + 4) % 5 );
    test( (who + 1) % 5 );

    dp->Release();

}


void DiningPh::test( long i ) {

    if ( ( state[ (i + 4) % 5 ] != Eating ) && 
         ( state[ i ] == Hungry ) && 
         ( state[ (i + 1) % 5] != Eating ) ) {
        state[ i ] = Eating;
        self[ i ]->Signal( dp );

    }

}

void DiningPh::print() {

    for ( int i = 0; i < 5; i++ ) {
        printf( "Philosopher %d is %s \n", i + 1, (state[i]==Hungry)?"Hungry":(state[i]==Thinking)?"Thinking":"Eating");

    }

}

