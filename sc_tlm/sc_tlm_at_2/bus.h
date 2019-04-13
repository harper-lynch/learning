#ifndef BUS_H
#define BUS_H

#include "utilities.h"
#include "tlm_utils/multi_passthrough_initiator_socket.h"
#include "tlm_utils/multi_passthrough_target_socket.h"

SC_MODULE(Bus) {
    tlm_utils::multi_passthrough_target_socket<Bus> targ_socket;
    tlm_utils::multi_passthrough_initiator_socket<Bus> init_socket;

    SC_CTOR(Bus):targ_socket("targ_socket"),init_socket("init_socket") {
        targ_socket.register_nb_transport_fw(this, &Bus::nb_transport_fw);
        targ_socket.register_b_transport(this, &Bus::b_transport);
        targ_socket.register_get_direct_mem_ptr(this, &Bus::get_direct_mem_ptr);
        targ_socket.register_transport_dbg(this, &Bus::transport_dbg);

        init_socket.register_nb_transport_bw(this, &Bus::nb_transport_bw);
        init_socket.register_invalidate_direct_mem_ptr(this, &Bus::invalidate_direct_mem_ptr);
    }

    virtual tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay) {

        assert ((unsigned)id < targ_socket.size());
        m_id_map[&trans] = id;
        sc_dt::uint64 address = trans.get_address();
        sc_dt::uint64 masked_address;
        unsigned int target_nr = decode_address(address, masked_address);

        if ( target_nr < init_socket.size() ) {
            trans.set_address(masked_address);
            tlm::tlm_sync_enum status = init_socket[target_nr]->nb_transport_fw(trans, phase, delay);

            if (status == tlm::TLM_COMPLETED) {
                trans.set_address(address);
            }
            return status;
        }
        else {
            return tlm::TLM_COMPLETED;
        }
    }

    virtual tlm::tlm_sync_enum nb_transport_bw(int id, tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay) {

        assert((unsigned)id < init_socket.size());
        sc_dt::uint64 address = trans.get_address();
        trans.set_address(compose_address(id, address));
        return targ_socket[m_id_map[&trans]]->nb_transport_bw(trans,phase,delay);

    }


    virtual void b_transport(int id, tlm::tlm_generic_payload &trans, sc_time &delay) {
 
        assert((unsigned)id < targ_socket.size());
        sc_dt::uint64 address = trans.get_address();
        sc_dt::uint64 masked_address;
        unsigned int target_nr = decode_address(address, masked_address);

        if (target_nr < init_socket.size()) {
            trans.set_address(masked_address);
            init_socket[target_nr]->b_transport(trans,delay);
            trans.set_address(address);
        }
    }


    virtual bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload &trans, tlm::tlm_dmi &dmi_data) {

        sc_dt::uint64 masked_address;
        unsigned int target_nr = decode_address(trans.get_address(), masked_address);
        if (target_nr >= init_socket.size() ) {
            return false;
        }

        trans.set_address(masked_address);

        bool status = init_socket[target_nr]->get_direct_mem_ptr(trans,dmi_data);
        dmi_data.set_start_address(compose_address(target_nr,dmi_data.get_start_address()));
        dmi_data.set_end_address(compose_address(target_nr,dmi_data.get_end_address()));

        return status;
    }

    virtual unsigned int transport_dbg(int id, tlm::tlm_generic_payload &trans) {

        sc_dt::uint64 masked_address;
        unsigned int target_nr = decode_address(trans.get_address(), masked_address);
        if (target_nr >= init_socket.size()) {
            return 0;
        }

        trans.set_address(masked_address);

        return init_socket[target_nr]->transport_dbg(trans);
    }

    virtual void invalidate_direct_mem_ptr(int id, sc_dt::uint64 start_range, sc_dt::uint64  end_range) {
        sc_dt::uint64 bw_start_range = compose_address(id, start_range);
        sc_dt::uint64 bw_end_range = compose_address(id, end_range);

        for(unsigned int i=0; i<targ_socket.size(); i++) {
            targ_socket[i]->invalidate_direct_mem_ptr(bw_start_range,bw_end_range);
        }
    }

    inline unsigned int decode_address(sc_dt::uint64 address, sc_dt::uint64 &masked_address) {
        unsigned int target_nr = static_cast<unsigned int>(address & 0x3);
        masked_address = address ;
        return target_nr;
    }

    inline sc_dt::uint64 compose_address(unsigned int target_nr, sc_dt::uint64 address) {
        return address;
    }

    std::map<tlm::tlm_generic_payload*,unsigned int> m_id_map;

};

#endif
