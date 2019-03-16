#ifndef SLAVE
#define SLAVE

#include "systemc.h"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

SC_MODULE(slave) {
    tlm_utils::simple_target_socket<slave> socket;

    SC_CTOR(slave):socket("socket") {
        socket.register_b_transport(this,&slave::b_transport);
    }

    virtual void b_transport(tlm::tlm_generic_payload &trans, sc_time & delay) {
        if ( trans.get_command() == tlm::TLM_WRITE_COMMAND ) {
            cout << "@" << sc_time_stamp() << " Write Command, address: 0x"<<hex<<trans.get_address()<<endl;
        }
        wait(delay);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};

#endif
