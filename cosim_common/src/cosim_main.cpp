#include <boost/foreach.hpp>
#include "schd_common.h"
#include "simd_common.h"
#include "cosim_adapter.h"
#include "schd_conv_ptree.h"

namespace schd {
   class core_data_t {
   public:
      std::size_t                              idx;
      std::string                              name;
      boost::optional<const boost_pt::ptree&>  pref_p;
      boost::optional<schd::schd_core_c &>     schd_core_p;
      boost::optional<schd::cosim_adapter_c &> adapter_p;
      boost::optional<simd::simd_sys_core_c &> simd_core_p;
   };

   typedef std::list<core_data_t> core_list_t;
} // namespace schd

int sc_main(
   int argc,
   char *argv[] ) {

   // Check command line arguments
   if( argc == 2 ) {
      std::string argv1 = argv[1];

      SCHD_REPORT_INFO( "cosim::cmdline" ) << "Preferences file: " << argv1;

      schd::schd_pref.load( argv1 );
   }
   else {
      SCHD_REPORT_ERROR( "cosim::cmdline" ) << "Incorrect command line arguments";
   }

   schd::schd_report.init(
         schd::schd_pref.get_pref( "report" ));

   schd::schd_time.init(
         schd::schd_pref.get_pref( "time" ));
   simd::simd_time.init(
         schd::schd_pref.get_pref( "time" ));

   sc_core::sc_set_time_resolution(
         schd::schd_time.res_sec,
         sc_core::SC_SEC );

   // Set trace for schd
   schd::schd_trace.init(
         schd::schd_pref.get_pref( "trace" ));

   schd::schd_trace.save_map(
         schd::schd_pref.get_pref( "threads" ));

   // Set trace for simd by reference
   simd::simd_trace.init(
         schd::schd_pref.get_pref( "trace" ),
         schd::schd_trace.tf );

   // Create SCHD PLANNER
   schd::schd_planner_c plan_i0(
         "planner" );
   plan_i0.init(
         schd::schd_pref.get_pref( "threads"   ),
         schd::schd_pref.get_pref( "tasks"     ),
         schd::schd_pref.get_pref( "executors" ));

   // Process core preferences
   schd::core_list_t core_list;
   schd::core_data_t core_data;

   // schd core
   core_data.idx  = 0;
   core_data.name = "schd";
   schd::schd_core_c *schd_core_raw_ptr = new schd::schd_core_c( core_data.name.c_str() );
   core_data.schd_core_p = boost::optional<schd::schd_core_c &>( *schd_core_raw_ptr );
   core_list.push_back( core_data );

   // simd cores
   boost::optional<const boost_pt::ptree&> simd_core_pref_p =
         schd::schd_pref.get_pref( "simd" );

   BOOST_FOREACH( const boost_pt::ptree::value_type& simd_core_pref_el, simd_core_pref_p.get()) {
      if( !simd_core_pref_el.first.empty()) {
         SCHD_REPORT_ERROR( "cosim::main" ) << " Incorrect structure of simd core preferences";
      }

      boost::optional<std::string>            name_p = simd_core_pref_el.second.get_optional<std::string>("name");
      boost::optional<const boost_pt::ptree&> pref_p = simd_core_pref_el.second.get_child_optional("pref");

      if( !name_p.is_initialized() ||
          !pref_p.is_initialized()) {
         SCHD_REPORT_ERROR( "schd::plan" ) << " Incorrect structure of simd core preferences";
      }

      if( core_list.end() != std::find_if(
            core_list.begin(),
            core_list.end(),
            [name_p]( schd::core_list_t::value_type& el )->bool{
               return name_p.get() == el.name; })) {
         SCHD_REPORT_ERROR( "schd::plan" ) << " Duplicate name: " << name_p.get();
      }

      core_data.idx    ++;
      core_data.name   = name_p.get();
      core_data.pref_p = pref_p;

      std::string adap_name = core_data.name + "_adap";
      schd::cosim_adapter_c *adapter_raw_ptr = new schd::cosim_adapter_c( adap_name.c_str() );
      core_data.adapter_p = boost::optional<schd::cosim_adapter_c &>( *adapter_raw_ptr );

      simd::simd_sys_core_c *simd_core_raw_ptr = new simd::simd_sys_core_c( core_data.name.c_str() );
      core_data.simd_core_p = boost::optional<simd::simd_sys_core_c &>( *simd_core_raw_ptr );

      core_list.push_back( core_data );
   } // BOOST_FOREACH( const boost_pt::ptree::value_type& simd_core_pref_el, simd_core_pref_p.get())

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


   BOOST_FOREACH( const schd::core_list_t::value_type& core_el, core_list ) {
      endpoint_pt.clear();
      endpoint_pt.put( "mask", "^" + core_el.name + ".*$" );    // regex
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
         schd::schd_pref.get_pref( "clock" ));

   // Create and connect clock and reset channels
   sc_core::sc_signal<bool> reset;
   sc_core::sc_signal<bool> clock;

   crm_i0.clock_o.bind(
         clock );
   crm_i0.reset_o.bind(
         reset );

   BOOST_FOREACH( const schd::core_list_t::value_type& core_el, core_list ) {
      if( core_el.idx == 0 ) { // SCHD core
         core_el.schd_core_p.get().init(
               schd::schd_pref.get_pref( "executors" ),
               schd::schd_pref.get_pref( "common"    ));

         // Connect co-sim mux and schd core
         mux_plan_core.vo.at( core_el.idx ).bind(
               core_el.schd_core_p.get().plan_eo );

         mux_core_plan.vi.at( core_el.idx ).bind(
               core_el.schd_core_p.get().plan_ei );

      } // if( core_el.schd )
      else { // a bunch of SIMD cores
         core_el.simd_core_p.get().init(
               core_el.pref_p );

         core_el.adapter_p.get().init(
               schd::schd_pref.get_pref( "executors" ),     // SCHD exec preferences
               core_el.pref_p );                            // SIMD core preferences

         // connect clock and reset channels
         core_el.simd_core_p.get().clock_i.bind(
               clock );
         core_el.simd_core_p.get().reset_i.bind(
               reset );

         core_el.adapter_p.get().clock_i.bind(
               clock );
         core_el.adapter_p.get().reset_i.bind(
               reset );

         // Connect ADAPTER to SIMD CORE
         core_el.adapter_p.get().event_i.bind(
               core_el.simd_core_p.get().event_ei );
         core_el.adapter_p.get().busr_i.bind(
               core_el.simd_core_p.get().busr_ei );
         core_el.adapter_p.get().busw_o.bind(
               core_el.simd_core_p.get().busw_eo );

         // Connect co-sim mux and ADAPTER
         mux_plan_core.vo.at( core_el.idx ).bind(
               core_el.adapter_p.get().plan_eo );

         mux_core_plan.vi.at( core_el.idx ).bind(
               core_el.adapter_p.get().plan_ei );
      } // if( core_el.schd ) ... else ...
   } // BOOST_FOREACH( const schd::core_list_t::value_type& core_el, core_list )

   // Init memory pool (virtual component) for all SIMD cores
   simd::simd_sys_pool.init(
         schd::schd_pref.get_pref( "pool" ));

   // Init data dump class for schd and for simd
   schd::schd_dump.init(
         schd::schd_pref.get_pref( "dump" ));

   simd::simd_dump.init(
         schd::schd_pref.get_pref( "dump" ));

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
