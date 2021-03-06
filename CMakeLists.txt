cmake_minimum_required( VERSION 3.16 )
project( "cosim"
   LANGUAGES CXX )

find_program( CCACHE_FOUND ccache )
if( CCACHE_FOUND )
	set( CMAKE_CXX_COMPILER_LAUNCHER ccache )
endif()

# Directories with project components
set(
   ABS_DIRS
   	# Directories with the simSCHD components
      "$ENV{SIMSCHD_HOME}/schd_common" 
      "$ENV{SIMSCHD_HOME}/schd_core"   
	   "$ENV{SIMSCHD_HOME}/schd_dump"   
      "$ENV{SIMSCHD_HOME}/schd_report" 
      "$ENV{SIMSCHD_HOME}/schd_systemc"
      "$ENV{SIMSCHD_HOME}/schd_trace"  
      "$ENV{SIMSCHD_HOME}/schd_pref"   
		"$ENV{SIMSCHD_HOME}/schd_time"
		       
# Directories with the simSIMD components		       
		"$ENV{SIMSIMD_HOME}/simd_common"     
      "$ENV{SIMSIMD_HOME}/simd_report"     
      "$ENV{SIMSIMD_HOME}/simd_systemc"    
      "$ENV{SIMSIMD_HOME}/simd_pref"       
      "$ENV{SIMSIMD_HOME}/simd_time"       
      "$ENV{SIMSIMD_HOME}/simd_trace"      
      "$ENV{SIMSIMD_HOME}/simd_dump"       
      "$ENV{SIMSIMD_HOME}/simd_sys_crm"    
      "$ENV{SIMSIMD_HOME}/simd_sys_core"   
      "$ENV{SIMSIMD_HOME}/simd_sys_xbar"   
      "$ENV{SIMSIMD_HOME}/simd_sys_eu"     
      "$ENV{SIMSIMD_HOME}/simd_sys_dm"     
      "$ENV{SIMSIMD_HOME}/simd_sys_stream" 
      "$ENV{SIMSIMD_HOME}/simd_sys_pool"   
      "$ENV{SIMSIMD_HOME}/simd_sys_scalar"
)
# Directories with the cosim components               
set(
   CMP_DIRS
		"cosim_common"  
)

# Include directories
message( "Boost   home: $ENV{BOOST_HOME}" )
message( "SystemC home: $ENV{SYSTEMC_HOME}" )
message( "matio   home: $ENV{MATIO_HOME}" )

# Initialize lists of source files and include directories
set( PRIVATE_CXX        "" )
set( PRIVATE_INC_DIR    "" )

# Parse directories
foreach( SRC_DIR ${CMP_DIRS} )
   include(
      "${CMAKE_CURRENT_LIST_DIR}/${SRC_DIR}/CMakeLists.txt"
   )
endforeach()

foreach( SRC_DIR ${ABS_DIRS} )
   include(
      "${SRC_DIR}/CMakeLists.txt"
   )
endforeach()

add_executable( "${PROJECT_NAME}" )

target_sources( "${PROJECT_NAME}"
   PRIVATE
      "${PRIVATE_CXX}"
)

target_include_directories( "${PROJECT_NAME}"
   PRIVATE
      "$ENV{SYSTEMC_HOME}/include"
      "$ENV{SYSTEMC_HOME}/include/sysc/utils"
      "$ENV{SYSTEMC_HOME}/include/sysc/kernel"
      "$ENV{BOOST_HOME}/include"
      "$ENV{MATIO_HOME}/include"
      "${PRIVATE_INC_DIR}"
)

target_link_directories( "${PROJECT_NAME}"
   PRIVATE
      "$ENV{LD_LIBRARY_PATH}"
      "$ENV{BOOST_HOME}/lib"
      "$ENV{SYSTEMC_HOME}/lib-linux64"
      "$ENV{MATIO_HOME}/lib"
)

target_link_libraries( "${PROJECT_NAME}"
   PRIVATE
      "stdc++"
      "m"
      "pthread"
      "systemc"
      "boost_system"
      "boost_regex"
      "matio"
)

set_target_properties( "${PROJECT_NAME}"
   PROPERTIES
      CXX_STANDARD 11
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS YES
)

if( CMAKE_COMPILER_IS_GNUCXX )
# Compiler 
	target_compile_options( "${PROJECT_NAME}" 
		PRIVATE 
			-Wall -Wpedantic -fexceptions -fPIC 
	)
	
	set( CMAKE_CXX_FLAGS_DEBUG 
		"${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -O0"
	)
	
	set( CMAKE_CXX_FLAGS_RELEASE 
		"${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer"
	)
	
# Linker	
	target_link_options( "${PROJECT_NAME}" 
		PRIVATE 
			-w -Wl,--no-undefined
	)
endif()

