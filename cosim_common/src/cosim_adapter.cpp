/*
 * cosim_adapter.cpp
 *
 *  Description: SCHD<->SIMD adapter for co-simulation
 */

#include <boost/random.hpp>
#include "cosim_adapter.h"
#include "schd_assert.h"
#include "schd_report.h"

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

   // Extract names of the simd core modules

   // Extract names of the schd exec blocks which correspond to simd
   BOOST_FOREACH( const boost_pt::ptree::value_type& exec_el, _exec_p.get()) {
      if( !exec_el.first.empty()) {
         SCHD_REPORT_ERROR( "schd::plan" ) << name() <<  " Incorrect exec structure";
      }

      boost::optional<std::string> name_p = exec_el.second.get_optional<std::string>("name");

      if( !name_p.is_initialized()) {
         SCHD_REPORT_ERROR( "schd::plan" ) << name() <<  " Incorrect exec structure";
      }

      // Initialize exec data
      exec_data_t exec_data;

      exec_list.emplace( std::make_pair( name_p.get(), exec_data ));
   } // BOOST_FOREACH( const boost_pt::ptree::value_type& exec_el, _exec_p.get())


   // Connect fifo channels with the corresponding exports
   plan_eo.bind( chn_plan_adap );
   plan_ei.bind( chn_adap_plan );
} // void cosim_adapter_c::init(

void cosim_adapter_c::exec_thrd(
      void ) {

   for(;;) {
      sc_core::wait();
   } // for(;;)
} // void cosim_adapter_c::exec_thrd(

} // namespace schd
