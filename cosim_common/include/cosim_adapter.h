/*
 * cosim_adapter.h
 *
 *  Description:
 *    Declaration of the system component:
 *       SCHD<->SIMD adapter for co-simulation
 */

#ifndef COSIM_COMMON_INCLUDE_COSIM_ADAPTER_H_
#define COSIM_COMMON_INCLUDE_COSIM_ADAPTER_H_

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include <systemc>
#include "schd_sig_ptree.h"
#include "simd_sig_ptree.h"

// Short alias for the namespace
namespace boost_pt = boost::property_tree;

namespace schd {

   SC_MODULE( cosim_adapter_c ) { // declare module class

   public:
      // ports to be connected to SIMD core
      sc_core::sc_in<bool> clock_i;    // Clock
      sc_core::sc_in<bool> reset_i;    // Reset
      sc_core::sc_port<sc_core::sc_fifo_in_if< simd::simd_sig_ptree_c>> event_i;    // SIMD Interrupt request fifo export through the input interface
      sc_core::sc_port<sc_core::sc_fifo_out_if<simd::simd_sig_ptree_c>> busw_o;     // SIMD Config output
      sc_core::sc_port<sc_core::sc_fifo_in_if< simd::simd_sig_ptree_c>> busr_i;     // SIMD Status input

      // ports to be connected to SCHD planner (via router)
      sc_core::sc_export<sc_core::sc_fifo_in_if <schd::schd_sig_ptree_c>> plan_ei;
      sc_core::sc_export<sc_core::sc_fifo_out_if<schd::schd_sig_ptree_c>> plan_eo;

      // Constructor declaration
      SC_CTOR( cosim_adapter_c );

      // Init/config declaration
      void init(
            boost::optional<const boost_pt::ptree&> schd_exec_p,        // SCHD exec preferences
            boost::optional<const boost_pt::ptree&> simd_core_p );      // SIMD core preferences

   private:
      // Process declarations
      void exec_thrd(
            void );

      // Channels
      sc_core::sc_fifo<schd_sig_ptree_c> chn_adap_plan;
      sc_core::sc_fifo<schd_sig_ptree_c> chn_plan_adap;

      static const int chn_adap_plan_size = 64;
      static const int chn_plan_adap_size = 64;
   };
}

#endif /* COSIM_COMMON_INCLUDE_COSIM_ADAPTER_H_ */
