#ifndef TOP
#define TOP

#include "systemc.h"
#include "master.cpp"
#include "slave.cpp"

SC_MODULE(top) {
    master *mst;
    slave  *slv;

    SC_CTOR(top) {
        mst = new master("mst");
        slv = new slave("slv");

        mst->socket.bind(slv->socket);
    }
};

#endif
