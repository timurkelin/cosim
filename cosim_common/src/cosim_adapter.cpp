/*
 * cosim_adapter.cpp
 *
 *  Description: SCHD<->SIMD adapter for co-simulation
 */

#include <boost/foreach.hpp>
#include "cosim_adapter.h"
#include "schd_conv_ptree.h"
#include "schd_dump.h"
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

   // Add virtual configuration dmeu
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

   schd_dump_buf_c<boost_pt::ptree> dump_buf_plan_i( std::string( name()) + ".plan_i" );
   schd_dump_buf_c<boost_pt::ptree> dump_buf_plan_o( std::string( name()) + ".plan_o" );
   schd_dump_buf_c<boost_pt::ptree> dump_buf_busr_i( std::string( name()) + ".busr_i" );
   schd_dump_buf_c<boost_pt::ptree> dump_buf_busw_o( std::string( name()) + ".busw_o" );
   schd_dump_buf_c<boost_pt::ptree> dump_buf_evnt_i( std::string( name()) + ".evnt_i" );

   for(;;) {
      schd_sig_ptree_c pt_out;

      sc_core::wait();

      bool plan_new = true;
      bool evnt_new = true;
      bool stat_new = true;

      if( !plan_pt.empty() ) { // New data from planner was fetched from fifo at the previous clock cycle

      }

      if( !evnt_pt.empty() ) { // New event was fetched from fifo at the previous clock cycle
         // Build full hierarchical event name
         boost::optional<std::string> src_p  = evnt_pt.get_optional<std::string>( "source"   );
         boost::optional<std::string> evnt_p = evnt_pt.get_optional<std::string>( "event_id" );

         if( !src_p.is_initialized() ||
             !evnt_p.is_initialized() ) {
            SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << " Incorrect event structure";
         }

         // Calculate hash
         std::string evnt_str  = core_name + "." + src_p.get() + "." + evnt_p.get();
         std::size_t evnt_hash = 0;
         boost::hash_combine(
               evnt_hash,
               evnt_str );

         auto evnt_cliq_it = evnt_cliq_list.find( evnt_hash );

         if( evnt_cliq_it == evnt_cliq_list.end()) {
            SCHD_REPORT_ERROR( "cosim::adapter" )
                  << name()
                  << " Unresolved event hash for: "
                  << evnt_str;
         }

         if( evnt_cliq_it->second.clique_p.is_initialized() ) { // Event is a member of a clique
            cliq_evnt_list_t::value_type &clique_r = evnt_cliq_it->second.clique_p.get();

            clique_r.second.evnt_count --;

            if( clique_r.second.evnt_count == 0 ) {
               BOOST_FOREACH( boost::optional<evnt_data_t &> evnt_ptr_el, clique_r.second.evnt_ptr_list ) {
                  evnt_ptr_el.get().job_hash = 0;
                  evnt_ptr_el.get().clique_p.reset();

                  boost_pt::ptree exec_pt;
                  boost_pt::ptree dst_list_pt;
                  dst_list_pt.push_back( std::make_pair( "", boost_pt::ptree().put( "", "planner" ) ));

                  exec_pt.put(       "src",  evnt_ptr_el.get().event );  // Name of this executor
                  exec_pt.put_child( "dst",  dst_list_pt             );  // Destination: planner

                  plan_eo->write( pt_out.set( exec_pt )); // Write data to the output

                  // Dump pt packets as they depart from the output of the block
                  dump_buf_plan_o.write( exec_pt, BUF_WRITE_LAST );
               }

               cliq_evnt_list.erase( clique_r.first );
            }
         } // if( evnt_cliq_it->second.clique_p.is_initialized() )
         else { // No clique specification. Report the completion straight away
            evnt_cliq_it->second.job_hash = 0;

            boost_pt::ptree exec_pt;
            boost_pt::ptree dst_list_pt;
            dst_list_pt.push_back( std::make_pair( "", boost_pt::ptree().put( "", "planner" ) ));

            exec_pt.put(       "src",  evnt_str    );  // Name of this executor
            exec_pt.put_child( "dst",  dst_list_pt );  // Destination: planner

            plan_eo->write( pt_out.set( exec_pt )); // Write data to the output

            // Dump pt packets as they depart from the output of the block
            dump_buf_plan_o.write( exec_pt, BUF_WRITE_LAST );
         } // if( evnt_cliq_it->second.clique_p.is_initialized() ) ... else ...
      } // if( !evnt_pt.empty() )

      if( !stat_pt.empty() ) { // New status was fetched from fifo at the previous clock cycle
         ; // Do nothing
      }

      // Read data from schd planner fifo
      while( plan_ei->num_available() != 0 && plan_new ) {
         plan_pt = plan_ei->read().get();

         boost::optional<std::string> dst_p  = plan_pt.get_optional<std::string>( "dst" );

         if( !dst_p.is_initialized() ) {
            SCHD_REPORT_ERROR( "cosim::adapter" ) << name() << " Incorrect data structure";
         }

         std::size_t evnt_hash = 0;
         boost::hash_combine(
               evnt_hash,
               dst_p.get() );

         auto evnt_cliq_it = evnt_cliq_list.find( evnt_hash );

         if( evnt_cliq_it == evnt_cliq_list.end()) {
            SCHD_REPORT_ERROR( "cosim::adapter" ) << name()
                                                  << " Unresolved event name: "
                                                  << dst_p.get();
         }

         // Set job hash
         boost::optional<std::string>      thrd_p = plan_pt.get_optional<std::string>("thread"); // Tread name
         boost::optional<std::string>      task_p = plan_pt.get_optional<std::string>("task");   // task name
         boost::optional<boost_pt::ptree&> para_p = plan_pt.get_child_optional("param");         // parameters ptree
         boost::optional<boost_pt::ptree&> optn_p = plan_pt.get_child_optional("options");       // options ptree

         if( !thrd_p.is_initialized() ||
             !task_p.is_initialized() ||
             !para_p.is_initialized() ||
             !optn_p.is_initialized()) {
            std::string plan_pt_str;

            SCHD_REPORT_ERROR( "schd::exec" ) << name()
                                              << " Incorrect data format from planner: "
                                              << pt2str( plan_pt, plan_pt_str );
         }

         // Get parameters id
         boost::optional<std::string> prid_p = para_p.get().get_optional<std::string>("id");

         if( !prid_p.is_initialized() ) {
            std::string plan_pt_str;

            SCHD_REPORT_ERROR( "schd::exec" ) << name()
                                              << " Incorrect data format from planner: "
                                              << pt2str( plan_pt, plan_pt_str );
         }

         // Update job hash
         std::string job_tag = schd_trace.job_comb(
               thrd_p.get(),
               task_p.get(),
               prid_p.get());

         evnt_cliq_it->second.job_hash = 0;
         boost::hash_combine(
               evnt_cliq_it->second.job_hash,
               job_tag  );

         // Get clique name if any
         boost::optional<std::string> cliq_p = optn_p.get().get_optional<std::string>("clique");

         if( cliq_p.is_initialized() &&
             cliq_p.get().size() != 0 ) {

            std::size_t cliq_hash = 0;
            boost::hash_combine(
                  cliq_hash,
                  cliq_p.get() );

            auto cliq_evnt_it = cliq_evnt_list.find( cliq_hash );

            if( cliq_evnt_it == cliq_evnt_list.end()) {
               cliq_data_t cliq_data;
               cliq_data.clique = cliq_p.get();

               bool done;
               std::tie( cliq_evnt_it, done ) = cliq_evnt_list.insert( cliq_evnt_list_t::value_type( cliq_hash, cliq_data ));
            }

            // Add event to the list of pointers
            cliq_evnt_it->second.evnt_count ++;
            cliq_evnt_it->second.evnt_ptr_list.push_back( boost::optional<evnt_data_t &>( evnt_cliq_it->second ));

            // Update pointer to the clique record
            evnt_cliq_it->second.clique_p = boost::optional<cliq_evnt_list_t::value_type &>( *cliq_evnt_it );
         }
         else {
            evnt_cliq_it->second.clique_p.reset();
         }

         // Dump pt packets as they arrive to the input of the block
         dump_buf_plan_i.write( plan_pt, BUF_WRITE_LAST );

         if( dst_p.get().find( core_name + ".config" ) == 0 ) {
            plan_pt.clear();
         }
         else {
            break;
         }
      }

      // Read data from simd event fifo
      if( event_i->num_available() != 0 && evnt_new ) {
         evnt_pt = event_i->read().get();

         // Dump pt packets as they arrive to the input of the block
         dump_buf_evnt_i.write( evnt_pt, BUF_WRITE_LAST );
      }

      // Read data from simd status fifo
      if( busr_i->num_available() != 0 && stat_new ) {
         stat_pt = busr_i->read().get();

         // Dump pt packets as they arrive to the input of the block
         dump_buf_busr_i.write( stat_pt, BUF_WRITE_LAST );
      }
   } // for(;;)
} // void cosim_adapter_c::exec_thrd(

} // namespace schd
