#ifndef TB
#define TB

#include "top.h"

int sc_main(int argc, char *argv[]) {
    Top top("top");
    sc_core::sc_start();

    cout <<"\n***** Message have been written to file output.txt*****\n";
    cout <<"***** Select 'Download files after run' to read file in EDA Playground*****\n\n";
    return 0; 
}

#endif
