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
        tlm::tlm_dmi           dmi_data;
        dmi_data.init();
        sc_time delay = sc_time(10,SC_NS);

        if (socket->get_direct_mem_ptr(*trans, dmi_data)) {
            char *memory_ptr=(char*)dmi_data.get_dmi_ptr();
            for(int i=0;i<10;i++) {
                sprintf(&memory_ptr[i],"%d",(i+2)%10);
            }
        }

        uint data[10] ;
        trans->set_command(tlm::TLM_READ_COMMAND);
        trans->set_address(0);
        trans->set_data_length(4);
        trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        socket->b_transport(*trans, delay);

        for(int i=0;i<10;i++) {
            unsigned char *data_ptr=trans->get_data_ptr();
            cout << i << " " << data_ptr[i] << endl;
        }
    }
};

#endif
