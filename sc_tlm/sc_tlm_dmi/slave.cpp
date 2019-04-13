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
    unsigned char memory[10];

    SC_CTOR(slave):socket("socket") {
        socket.register_b_transport(this,&slave::b_transport);
        socket.register_get_direct_mem_ptr(this,&slave::get_direct_mem_ptr);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload &trans, tlm::tlm_dmi &dmi_data) {
        dmi_data.set_dmi_ptr(memory);
        return true;
    }

    virtual void b_transport(tlm::tlm_generic_payload &trans, sc_time & delay) {
        sc_time process_time = sc_time(5, SC_NS);
        if ( trans.get_command() == tlm::TLM_READ_COMMAND ) {
            unsigned char *data_ptr = trans.get_data_ptr();
            for(int i=0;i<10;i++) {
                data_ptr[i] = memory[i];
                cout << "memory[" << i << "]=" << memory[i] << endl;
            }
        }
        wait(process_time);
        delay = delay + process_time;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};

#endif
