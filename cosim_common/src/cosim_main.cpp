#include <boost/foreach.hpp>
#include "schd_common.h"
#include "simd_common.h"
#include "cosim_adapter.h"
#include "schd_conv_ptree.h"

// Prefixes for the simd cores
std::vector<std::string> core_prefix_v = {
      "schd_",      // SCHD core is at( 0 )
      "simd_" };    // a bunch of SIMD cores can be placed here

int sc_main(
   int argc,
   char *argv[] ) {

   // Check command line arguments
   if( argc == 2 ) {
      std::string argv1 = argv[1];

      SCHD_REPORT_INFO( "cosim::cmdline" ) << "Preferences file: " << argv1;

      schd::schd_pref.load( argv1 );
      simd::simd_pref.load( argv1 );
   }
   else {
      SCHD_REPORT_ERROR( "cosim::cmdline" ) << "Incorrect command line arguments";
   }

   schd::schd_pref.parse(); // Parse schd preferences
   simd::simd_pref.parse(); // Parse simd preferences

   schd::schd_report.init(
         schd::schd_pref.report_p );

   schd::schd_time.init(
         schd::schd_pref.time_p );
   simd::simd_time.init(
         simd::simd_pref.time_p );

   sc_core::sc_set_time_resolution(
         schd::schd_time.res_sec,
         sc_core::SC_SEC );

   // Set trace for schd
   schd::schd_trace.init(
         schd::schd_pref.trace_p );

   schd::schd_trace.save_map(
         schd::schd_pref.thrd_p );

   // Set trace for simd by reference
   simd::simd_trace.init(
         simd::simd_pref.trace_p,
         schd::schd_trace.tf );

   // Create SCHD PLANNER
   schd::schd_planner_c plan_i0(
         "planner" );
   plan_i0.init(
         schd::schd_pref.thrd_p,
         schd::schd_pref.task_p,
         schd::schd_pref.exec_p );

   // Create and initialize co-simulation mux
   schd::schd_ptree_xbar_c mux_plan_core( "mux_plan_core" );
   schd::schd_ptree_xbar_c mux_core_plan( "mux_core_plan" );

   boost_pt::ptree mux_plan_core_pref_pt;
   boost_pt::ptree mux_core_plan_pref_pt;
   boost_pt::ptree core_list_pt;
   boost_pt::ptree plan_list_pt;
   boost_pt::ptree endpoint_pt;

   endpoint_pt.clear();
   endpoint_pt.put( "name", "planner" );
   plan_list_pt.push_back( std::make_pair( "", endpoint_pt ));

   BOOST_FOREACH( const std::string& core_el, core_prefix_v ) {
      endpoint_pt.clear();
      endpoint_pt.put( "mask", "^" + core_el + ".*$" );    // regex
      core_list_pt.push_back( std::make_pair( "", endpoint_pt ));
   }

   mux_plan_core_pref_pt.add_child( "src_list", plan_list_pt );
   mux_plan_core_pref_pt.add_child( "dst_list", core_list_pt );

   mux_core_plan_pref_pt.add_child( "src_list", core_list_pt );
   mux_core_plan_pref_pt.add_child( "dst_list", plan_list_pt );

   mux_plan_core.init(
         boost::optional<const boost_pt::ptree&>( mux_plan_core_pref_pt ));

   mux_core_plan.init(
         boost::optional<const boost_pt::ptree&>( mux_core_plan_pref_pt ));

   // Create channels and connect co-sim mux and planner
   sc_core::sc_fifo<schd::schd_sig_ptree_c> chn_core_plan( "chn_core_plan", 64 );
   sc_core::sc_fifo<schd::schd_sig_ptree_c> chn_plan_core( "chn_plan_core", 64 );

   plan_i0.core_o.bind( chn_plan_core );
   mux_plan_core.vi.at( 0 ).bind( chn_plan_core );

   mux_core_plan.vo.at( 0 ).bind( chn_core_plan );
   plan_i0.core_i.bind( chn_core_plan );

   // Create and initialise SIMD CRM instance
   simd::simd_sys_crm_c crm_i0(
         "crm");
   crm_i0.init(
         simd::simd_pref.clock_p );

   boost::optional<schd::schd_core_c &>                  schd_core_p;
   std::vector<boost::optional<schd::cosim_adapter_c &>> adapter_pv(   core_prefix_v.size() );
   std::vector<boost::optional<simd::simd_sys_core_c &>> simd_core_pv( core_prefix_v.size() );

   for( std::size_t core_idx = 0; core_idx < core_prefix_v.size(); core_idx ++ ) {
      std::string core_name = core_prefix_v.at( core_idx ) + "core";

      if( core_idx == 0 ) { // SCHD core is at( 0 )
         schd::schd_core_c *schd_core_ptr = new schd::schd_core_c( core_name.c_str() );
         schd_core_p = boost::optional<schd::schd_core_c &>( *schd_core_ptr );

         // Initialise SCHD CORE
         schd_core_p.get().init(
               schd::schd_pref.exec_p,
               schd::schd_pref.cres_p );

         // Connect co-sim mux and schd core
         mux_plan_core.vo.at( 0 ).bind(
               schd_core_p.get().plan_eo );

         mux_core_plan.vi.at( 0 ).bind(
               schd_core_p.get().plan_ei );

      } // if( core_idx == 0 )
      else { // a bunch of SIMD cores
         std::string adap_name = core_prefix_v.at( core_idx ) + "adap";
         schd::cosim_adapter_c *adapter_ptr = new schd::cosim_adapter_c( adap_name.c_str() );
         adapter_pv.at( core_idx ) = boost::optional<schd::cosim_adapter_c &>( *adapter_ptr );

         simd::simd_sys_core_c *simd_core_ptr = new simd::simd_sys_core_c( core_name.c_str() );
         schd_core_p = boost::optional<simd::simd_sys_core_c &>( *simd_core_ptr );

         boost::optional<boost_pt::ptree &> simd_core_pref_p =

         simd_core_i0.init(
               simd::simd_pref.core_p );

      } // if( core_idx == 0 ) ... else ...
   } // for( std::size_t core_idx = 0; core_idx < core_prefix_v.size(); core_idx ++ )


   // Create and initialise SIMD CORE instance
   simd::simd_sys_core_c simd_core_i0(
         "simd_core" );
   simd_core_i0.init(
         simd::simd_pref.core_p );

   // Create and initialise ADAPTER instance
   schd::cosim_adapter_c adapter_i0(
         "adapter" );
   adapter_i0.init(
         schd::schd_pref.exec_p,    // SCHD exec preferences
         simd::simd_pref.core_p );  // SIMD core preferences

   // Create and connect clock and reset channels
   sc_core::sc_signal<bool> reset;
   sc_core::sc_signal<bool> clock;

   crm_i0.clock_o.bind(
         clock );
   crm_i0.reset_o.bind(
         reset );

   simd_core_i0.clock_i.bind(
         clock );
   simd_core_i0.reset_i.bind(
         reset );

   adapter_i0.clock_i.bind(
         clock );
   adapter_i0.reset_i.bind(
         reset );

   // Connect ADAPTER to SIMD CORE
   adapter_i0.event_i.bind(
         simd_core_i0.event_ei );
   adapter_i0.busr_i.bind(
         simd_core_i0.busr_ei );
   adapter_i0.busw_o.bind(
         simd_core_i0.busw_eo );

   // Connect co-sim mux and ADAPTER
   mux_plan_core.vo.at( 1 ).bind(
         adapter_i0.plan_eo );

   mux_core_plan.vi.at( 1 ).bind(
         adapter_i0.plan_ei );

   // Init SIMD memory pool (virtual component)
   simd::simd_sys_pool.init(
         simd::simd_pref.pool_p );

   // Init data dump class for schd and for simd
   schd::schd_dump.init(
         schd::schd_pref.dump_p );

   simd::simd_dump.init(
         simd::simd_pref.dump_p );

   // Invoke the simulation
   if( schd::schd_time.end_sec != 0.0 ) {
      sc_core::sc_start(
            schd::schd_time.end_sec,
            sc_core::SC_SEC );
   }
   else {
      sc_core::sc_start();
   }

   SCHD_REPORT_INFO( "cosim::main" ) << "Done.";

   // Ensure that all the dump files are closed before exiting
   schd::schd_dump.close_all();
   simd::simd_dump.close_all();

   return 0;
}
