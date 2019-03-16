#ifndef MASTER
#define MASTER

#include "systemc.h"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

SC_MODULE(master) {
    tlm_utils::simple_initiator_socket<master> socket;

    SC_CTOR(master):socket("socket") {
        SC_THREAD(thread_process);
    }

    void thread_process(){
        tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload;
        sc_time delay = sc_time(10,SC_NS);

        for(int i=0;i<10;i++) {
            uint data = 0xFF00 | i;
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(i*4);
            trans->set_data_length(4);
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
            trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            socket->b_transport(*trans, delay);
        }
    }
};

#endif
