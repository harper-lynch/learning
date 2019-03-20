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
    int BW_METHOD ; // 0: complete; 1:accepted; 2: updated
    sc_event nb_phase_end;
    sc_event send_begin_req;
    sc_event send_end_resp;

    tlm::tlm_generic_payload *trans;
    tlm::tlm_sync_enum status;
    tlm::tlm_phase phase;
    sc_time delay = sc_time(10,SC_NS);

    SC_CTOR(master):socket("socket") {
        SC_THREAD(thread_process);
        SC_THREAD(send_begin_req_phase);
        SC_THREAD(send_end_resp_phase);
        socket.register_nb_transport_bw(this,&master::nb_transport_bw);
        BW_METHOD = 0;
    }

    void thread_process(){
        wait(10,SC_NS);
        for(int i=0;i<20;i++) {
            uint data = 0xFF00 | i;
            trans = new tlm::tlm_generic_payload;
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(i*4);
            trans->set_data_length(4);
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));

            cout<<endl<<endl;
            cout<<i<<":"<<endl;
            send_begin_req.notify();

            wait(nb_phase_end);
            BW_METHOD = (BW_METHOD+1)%3;
            wait(10,SC_NS);
        }
    }


    void send_begin_req_phase() {
        while(1) {
            wait(send_begin_req);
            phase = tlm::BEGIN_REQ;
            cout<<"master send BEGIN_REQ through FW"<<endl;
            status=socket->nb_transport_fw(*trans, phase, delay);

            switch (status) {
                case tlm::TLM_COMPLETED:  {
                                              nb_phase_end.notify();
                                              break;
                                          }
                case tlm::TLM_ACCEPTED :  {
                                              break;
                                          }
                case tlm::TLM_UPDATED  :  {
                                              if ( phase == tlm::BEGIN_RESP ) {
                                                  send_end_resp.notify();
                                              }
                                              break;
                                          }
                default : break ;
            } 
        }
    }

    void send_end_resp_phase() {
        while(1) {
            wait(send_end_resp);
   
            phase = tlm::END_RESP;
            cout<<"master send END_RESP through FW"<<endl;
            status=socket->nb_transport_fw(*trans, phase, delay);
            if ( status == tlm::TLM_COMPLETED ) {
                nb_phase_end.notify();
            }
        }
    }

    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay) {
        if ( phase == tlm::END_REQ ) {
            if ( BW_METHOD == 0 ) {
                cout<<"master send COMPLETED through return"<<endl;
                nb_phase_end.notify();
                return tlm::TLM_COMPLETED;
            }
            else {
                cout<<"master send ACCEPTED through return"<<endl;
                return tlm::TLM_ACCEPTED;
            }
        }
        else if ( phase == tlm::BEGIN_RESP ) {
            if ( BW_METHOD == 0 ) {
                nb_phase_end.notify();
                cout<<"master send COMPLETED through return"<<endl;
                return tlm::TLM_COMPLETED;
            }
            else if ( BW_METHOD == 1 ) {
                send_end_resp.notify();
                cout<<"master send ACCEPTED through return"<<endl;
                return tlm::TLM_ACCEPTED;
            }
            else {
                cout<<"master send END_RESP through return"<<endl;
                nb_phase_end.notify();
                phase = tlm::END_RESP;
                return tlm::TLM_UPDATED;
            }
        }
        else {
            return tlm::TLM_COMPLETED;
        }
    }
};

#endif
