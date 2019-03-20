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
    int FW_METHOD ; // 0:completed, 1:accepted, 2:updated_with_end_req, 3:updated_with_begin_resp
    sc_event send_end_req;
    sc_event send_begin_resp;

    tlm::tlm_sync_enum status;
    tlm::tlm_generic_payload *trans;
    tlm::tlm_phase phase;
    sc_time delay ;

    SC_CTOR(slave):socket("socket") {
        SC_THREAD(send_end_req_phase);
        SC_THREAD(send_begin_resp_phase);
        socket.register_nb_transport_fw(this,&slave::nb_transport_fw);
        FW_METHOD = 0;
    }

    void send_end_req_phase() {
        while(1) {
            wait(send_end_req);
            phase = tlm::END_REQ;

            cout<<"        slave send END_REQ through BW"<<endl;
            status = socket->nb_transport_bw(*trans, phase, delay);
            if ( status == tlm::TLM_ACCEPTED ) {
                send_begin_resp.notify();
            }
        }
    }

    void send_begin_resp_phase() {
        while(1) {
            wait(send_begin_resp);
            phase = tlm::BEGIN_RESP;
            cout<<"        slave send BEGIN_RESP through BW"<<endl;
            status = socket->nb_transport_bw(*trans, phase, delay);
        }
    }

    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay) {
        this->trans = &trans;
        this->phase = phase;
        this->delay = delay;

        if ( phase == tlm::BEGIN_REQ ) {
            FW_METHOD = (FW_METHOD+1)%4;

            if ( FW_METHOD == 0 ) {
                cout<<"        slave send COMPLETED through return"<<endl;
                return tlm::TLM_COMPLETED;
            }
            else if ( FW_METHOD == 1 ) {
                send_end_req.notify();
                cout<<"        slave send ACCEPTED through return"<<endl;
                return tlm::TLM_ACCEPTED;
            }
            else if ( FW_METHOD == 2 ) {
                phase = tlm::END_REQ;
                cout<<"        slave send END_REQ through return"<<endl;
                send_begin_resp.notify();
                return tlm::TLM_UPDATED;
            }
            else {
                phase = tlm::BEGIN_RESP;
                cout<<"        slave send BEGIN_RESP through return"<<endl;
                return tlm::TLM_UPDATED;
            }
        } 
        else if ( phase == tlm::END_RESP ) {
            cout<<"        slave send COMPLETED through return"<<endl;
            return tlm::TLM_COMPLETED;
        }
        else {
            cout<<"        slave send COMPLETED through return"<<endl;
            return tlm::TLM_COMPLETED;
        }
    }
};

#endif
