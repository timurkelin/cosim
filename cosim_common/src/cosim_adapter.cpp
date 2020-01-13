/*
 * cosim_adapter.cpp
 *
 *  Description: SCHD<->SIMD adapter for co-simulation
 */

#include <boost/foreach.hpp>
#include "cosim_adapter.h"
#include "schd_assert.h"
#include "schd_report.h"
#include "schd_trace.h"

namespace schd {

SC_HAS_PROCESS( schd::cosim_adapter_c );
cosim_adapter_c::cosim_adapter_c(
      sc_core::sc_module_name nm )
   : sc_core::sc_module( nm )
   , clock_i( "clock_i" )
   , reset_i( "reset_i" )
   , event_i( "event_i" )
   , busw_o(  "busw_o"  )
   , busr_i(  "busr_i"  )
   , plan_ei( "plan_ei" )
   , plan_eo( "plan_eo" )
   , chn_adap_plan( "chn_adap_plan", chn_adap_plan_size )
   , chn_plan_adap( "chn_plan_adap", chn_plan_adap_size ) {

   // Process registrations
   SC_CTHREAD( exec_thrd, clock_i.pos() ); //  Synchronous thread for data processing
   reset_signal_is( reset_i, true );
} // cosim_adapter_c::cosim_adapter_c(

void cosim_adapter_c::init(
      boost::optional<const boost_pt::ptree&> schd_exec_p,        // SCHD exec preferences
      boost::optional<const boost_pt::ptree&> simd_core_p ) {     // SIMD core preferences

   // Extract simd core name from the adapter name
   std::string adpt_name = name();
   std::size_t core_pos  = adpt_name.rfind( "_adpt" );

   if( core_pos == std::string::npos ) {
      SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << " Incorrect name";
   }

   core_name = adpt_name.substr( 0, core_pos );

   // Extract names of the simd core modules from simd core preferences
   std::list<std::string> dmeu_list; // List of DME/EU blocks in the simd core

   BOOST_FOREACH( const boost_pt::ptree::value_type& dmeu_el, simd_core_p.get() ) {
      if( !dmeu_el.first.empty()) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << "Incorrect format";
      }

      try {
         dmeu_list.push_back(
               core_name + "." + dmeu_el.second.get<std::string>("name"));
      }
      catch( const boost_pt::ptree_error& err ) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << err.what();
      }
      catch( const std::exception& err ) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << err.what();
      }
      catch( ... ) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << "Unexpected";
      }
   } // BOOST_FOREACH( const boost_pt::ptree::value_type& dmeu_el, simd_core_p.get() )

   // Add virtual configuring dmeu
   dmeu_list.push_back( core_name + ".config" );

   // Extract names of the schd exec blocks which correspond to simd
   BOOST_FOREACH( const boost_pt::ptree::value_type& exec_el, schd_exec_p.get()) {
      if( !exec_el.first.empty()) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() <<  " Incorrect structure";
      }

      boost::optional<std::string> name_p = exec_el.second.get_optional<std::string>("name");

      if( !name_p.is_initialized() ) {
         SCHD_REPORT_ERROR( "cosim::adapter" ) << name() <<  " Incorrect exec name";
      }

      // Check if the core.dmeu is in the dmeu list
      if( dmeu_list.end() == std::find_if(
            dmeu_list.begin(),
            dmeu_list.end(),
            [name_p]( const std::string& el )->bool {
               return el.find( name_p.get() ) == 0; } )) {
         continue;
      }

      evnt_data_t evnt_data;
      std::size_t evnt_hash = 0;
      boost::hash_combine(
            evnt_hash,
            name_p.get() );

      if( evnt_cliq_list.find( evnt_hash ) != evnt_cliq_list.end()) {
         SCHD_REPORT_ERROR( "cosim::adapter" )
               << name()
               << " Duplicate hash for: "
               << name_p.get();
      }

      evnt_data.event = name_p.get();

      evnt_cliq_list.insert({ evnt_hash, evnt_data });
   } // BOOST_FOREACH( const boost_pt::ptree::value_type& exec_el, schd_exec_p.get())

   // Connect fifo channels with the corresponding exports
   plan_eo.bind( chn_plan_adap );
   plan_ei.bind( chn_adap_plan );
} // void cosim_adapter_c::init(

void cosim_adapter_c::add_trace(
      sc_core::sc_trace_file* tf,
      const std::string& top_name ) {
   std::string mod_name = top_name + "." + name() + ".";

   BOOST_FOREACH( evnt_cliq_list_t::value_type& evnt_el, evnt_cliq_list ) {
      sc_core::sc_trace(
            tf,
            &( evnt_el.second.job_hash ),
            mod_name + evnt_el.second.event + ".job_hash" );
   }
} // cosim_adapter_c::add_trace(

void cosim_adapter_c::exec_thrd(
      void ) {

   for(;;) {
      sc_core::wait();

      bool plan_new = true;
      bool evnt_new = true;
      bool stat_new = true;

      if( !plan_pt.empty() ) { // New data from planner was fetched at the previous clock cycle

      }

      if( !evnt_pt.empty() ) { // New event was fetched at the previous clock cycle
         // Build full hierarchical event name
         boost::optional<std::string> src_p  = evnt_pt.get_optional<std::string>( "source"   );
         boost::optional<std::string> evnt_p = evnt_pt.get_optional<std::string>( "event_id" );

         if( !src_p.is_initialized() ||
             !evnt_p.is_initialized() ) {
            SCHD_REPORT_ERROR( "cosim::adapter" ) << " Incorrect event structure";
         }

         // Calculate hash


      }

      if( !stat_pt.empty() ) { // New status was fetched at the previous clock cycle
         ; // Do nothing
      }

      // Read data from schd planner fifo
      if( plan_ei->num_available() != 0 && plan_new ) {
         plan_pt = plan_ei->read().get();
      }

      // Read data from simd event fifo
      if( event_i->num_available() != 0 && evnt_new ) {
         evnt_pt = event_i->read().get();
      }

      // Read data from simd status fifo
      if( busr_i->num_available() != 0 && stat_new ) {
         stat_pt = busr_i->read().get();
      }
   } // for(;;)
} // void cosim_adapter_c::exec_thrd(

} // namespace schd
