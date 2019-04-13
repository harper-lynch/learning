#ifndef INITIATOR_H
#define INITIATOR_H

#include "utilities.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"

SC_MODULE(Initiator) {
    tlm_utils::simple_initiator_socket<Initiator> socket;

    SC_CTOR(Initiator) : socket("socket"), request_in_progress(0), m_peq(this, &Initiator::peq_cb) {
        socket.register_nb_transport_bw(this, &Initiator::nb_transport_bw);

        SC_THREAD(thread_process);
    }

    void thread_process() {
        tlm::tlm_generic_payload *trans;
        tlm::tlm_phase phase;
        sc_time delay;

        for(int i=0;i<1000;i++) {
            int adr = rand();
            tlm::tlm_command cmd = static_cast<tlm::tlm_command> (rand()%2);
            if (cmd == tlm::TLM_WRITE_COMMAND) {
                data[i%16] = rand();
            }
            trans = m_mm.allocate();
            trans->acquire();
            trans->set_command(cmd);
            trans->set_address(adr);
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data[i%16]));
            trans->set_data_length(4);
            trans->set_streaming_width(4);
            trans->set_byte_enable_ptr(0);
            trans->set_dmi_allowed(false);
            trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            if (request_in_progress) {
                wait(end_request_event);
            }
            request_in_progress = trans;
            phase = tlm::BEGIN_REQ;

            delay = sc_time(rand_ps(), SC_PS);

            fout << hex << adr << " " << name() << " new, cmd=" << (cmd ? 'W' : 'R') << ", data=" << hex << data[i%16] << " at time " << sc_time_stamp() << endl;

            tlm::tlm_sync_enum status;
            status = socket->nb_transport_fw(*trans, phase, delay);
            if (status == tlm::TLM_UPDATED) {
                m_peq.notify(*trans,phase,delay);
            }
            else if (status == tlm::TLM_COMPLETED) {
                request_in_progress = 0;
                check_transaction(*trans);
            }
            wait(sc_time(rand_ps(),SC_PS));
        }

        wait(100,SC_NS);

        trans = m_mm.allocate();
        trans->acquire();
        trans->set_command(tlm::TLM_WRITE_COMMAND);
        trans->set_address(0);
        trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data[0]));
        trans->set_data_length(4);
        trans->set_streaming_width(4);
        trans->set_byte_enable_ptr(0);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        delay = sc_time(rand_ps(),SC_PS);

        fout << "Caling b_transport at "<< sc_time_stamp()<<" with delay = "<<delay<<endl;
        socket->b_transport(*trans,delay);
        check_transaction(*trans);
    }

    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay) {
        m_peq.notify(trans, phase, delay);
        return tlm::TLM_ACCEPTED;
    }

    void peq_cb(tlm::tlm_generic_payload &trans, const tlm::tlm_phase &phase) {
        #ifdef DEBUG
            if (phase == tlm::END_REQ) {
                fout << hex << trans.get_address() << " " << name() << " END_REQ at " << sc_time_stamp() << endl;
            }
            else if (phase == tlm::BEGIN_RESP) {
                fout << hex << trans.get_address() << " " << name() << " BEGIN_RESP at " << sc_time_stamp() << endl;
            }
        #endif

        if (phase == tlm::END_REQ || (&trans == request_in_progress && phase == tlm::BEGIN_RESP)) {
            request_in_progress = 0;
            end_request_event.notify();
        }
        else if (phase == tlm::BEGIN_REQ || phase == tlm::END_RESP) {
            SC_REPORT_FATAL("TLM-2", "Illegal transaction phase received by initiator");
        }

        if (phase == tlm::BEGIN_RESP) {
            check_transaction(trans);
            tlm::tlm_phase fw_phase = tlm::END_RESP;
            sc_time delay = sc_time(rand_ps(), SC_PS);
            socket->nb_transport_fw(trans,fw_phase,delay);
        }
    }

    void check_transaction(tlm::tlm_generic_payload &trans) {
        if (trans.is_response_error()) {
            char txt[100];
            sprintf(txt,"Transaction returned with error, response status = %s", trans.get_response_string().c_str());
            SC_REPORT_ERROR("TLM-2",txt);
        }

        tlm::tlm_command cmd = trans.get_command();
        sc_dt::uint64 adr = trans.get_address();
        int *ptr = reinterpret_cast<int*>(trans.get_data_ptr());
        
        fout << hex << adr << " " << name() << " check, cmd=" << (cmd?'W':'R') << ", data=" << hex << *ptr << " at time " << sc_time_stamp() << endl;

        trans.release();
    }

    mm m_mm;
    int data[16];
    tlm::tlm_generic_payload *request_in_progress;
    sc_event end_request_event;
    tlm_utils::peq_with_cb_and_phase<Initiator> m_peq;
};

#endif
