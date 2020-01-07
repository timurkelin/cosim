/*
 * simd_sys_scalar_run.cpp
 *
 *  Description:
 *    System components: Scalar processor dummy
 */

#include <boost/random.hpp>
#include "simd_sys_scalar.h"
#include "simd_assert.h"
#include "simd_report.h"
#include "simd_trace.h"

namespace simd {

unsigned int n_test;

void simd_sys_scalar_c::init(
      boost::optional<const boost_pt::ptree&> pref_p ) {
}

void simd_sys_scalar_c::exec_thrd(
      void ) {

   for(;;) {
      sc_core::wait();
   } // for(;;)
}

} // namespace simd
