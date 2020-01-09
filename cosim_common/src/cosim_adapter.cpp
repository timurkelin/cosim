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

   // Extract simd core name for the adapter name
   std::string adpt_name = name();
   std::size_t core_pos  = adpt_name.rfind( "_adpt" );

   if( core_pos == std::string::npos ) {
      SCHD_REPORT_ERROR( "cosim::adapter" ) << " Incorrect name: " << adpt_name;
   }

   core_name = adpt_name.substr( 0, core_pos );

   // Extract names of the simd core modules

   // Extract names of the schd exec blocks which correspond to simd

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
