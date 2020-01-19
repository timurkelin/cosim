MAKEFILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_ROOT_DIR = $(patsubst %/,%,$(dir $(MAKEFILE_PATH)))

# Directories with the simSCHD components
CMP_SCHD_DIRS = $(SIMSCHD_HOME)/schd_common  \
               $(SIMSCHD_HOME)/schd_core    \
	           $(SIMSCHD_HOME)/schd_dump    \
               $(SIMSCHD_HOME)/schd_report  \
               $(SIMSCHD_HOME)/schd_systemc \
               $(SIMSCHD_HOME)/schd_trace   \
               $(SIMSCHD_HOME)/schd_pref    \
		       $(SIMSCHD_HOME)/schd_time
		       
# Directories with the simSIMD components		       
CMP_SIMD_DIRS = $(SIMSIMD_HOME)/simd_common     \
               $(SIMSIMD_HOME)/simd_report     \
               $(SIMSIMD_HOME)/simd_systemc    \
               $(SIMSIMD_HOME)/simd_pref       \
               $(SIMSIMD_HOME)/simd_time       \
               $(SIMSIMD_HOME)/simd_trace      \
               $(SIMSIMD_HOME)/simd_dump       \
               $(SIMSIMD_HOME)/simd_sys_crm    \
               $(SIMSIMD_HOME)/simd_sys_core   \
               $(SIMSIMD_HOME)/simd_sys_xbar   \
               $(SIMSIMD_HOME)/simd_sys_eu     \
               $(SIMSIMD_HOME)/simd_sys_dm     \
               $(SIMSIMD_HOME)/simd_sys_stream \
               $(SIMSIMD_HOME)/simd_sys_pool   \
               $(SIMSIMD_HOME)/simd_sys_scalar

# Directories with the cosim components               
CMP_COSIM_DIRS = $(PROJ_ROOT_DIR)/cosim_common

CMP_DIRS = $(CMP_COSIM_DIRS) $(CMP_SCHD_DIRS) $(CMP_SIMD_DIRS)

# Include folders
INC_LIB_DIRS = $(SYSTEMC_HOME)/include \
			   $(SYSTEMC_HOME)/include/sysc/utils \
			   $(SYSTEMC_HOME)/include/sysc/kernel \
			   $(BOOST_HOME)/include \
			   $(MATIO_HOME)/include

INC_COSIM_DIRS = $(addsuffix /include,$(CMP_COSIM_DIRS))
INC_SCHD_DIRS  = $(addsuffix /include,$(CMP_SCHD_DIRS))
INC_SIMD_DIRS  = $(addsuffix /include,$(CMP_SIMD_DIRS))

INC_CXX_DIRS := $(INC_LIB_DIRS) $(INC_COSIM_DIRS) $(INC_SCHD_DIRS) $(INC_SIMD_DIRS)

# Compiler flags
CXXFLAGS = -c -Wall -std=gnu++11 -fexceptions -fPIC -pedantic

ifndef CONFIG
  CONFIG = Release
endif
  
ifeq "$(CONFIG)" "Release"
  OBJ_DIR   = $(PROJ_ROOT_DIR)/build/Release/obj
  EXE_DIR   = $(PROJ_ROOT_DIR)/build/Release/out
  CXXFLAGS += -O -DNDEBUG -fomit-frame-pointer
  LDFLAGS   = 
  LDLIBS    =  
endif

ifeq "$(CONFIG)" "Profile"
  OBJ_DIR   = $(PROJ_ROOT_DIR)/build/Profile/obj
  EXE_DIR   = $(PROJ_ROOT_DIR)/build/Profile/out
  CXXFLAGS += -pg -O -DNDEBUG -fno-omit-frame-pointer
  LDFLAGS   = -pg
  LDLIBS    =
endif

ifeq "$(CONFIG)" "Debug"
  OBJ_DIR   = $(PROJ_ROOT_DIR)/build/Debug/obj
  EXE_DIR   = $(PROJ_ROOT_DIR)/build/Debug/out
  CXXFLAGS += -gdwarf-2 -O0 -fno-omit-frame-pointer
  LDFLAGS   = -gdwarf-2
  LDLIBS    =  
endif

LDFLAGS += -w -Wl,--no-undefined \
           -L$(LD_LIBRARY_PATH) \
           -L$(BOOST_HOME)/lib \
           -L$(SYSTEMC_HOME)/lib-linux64 \
           -L$(MATIO_HOME)/lib

# Libraries to link against
LDLIBS += -lstdc++ \
          -lm \
          -lpthread \
          -lsystemc \
		  -lboost_system \
		  -lboost_regex \
		  -lmatio

# Specify variables for Boilermake
TARGET := cosim
BUILD_DIR := ${OBJ_DIR}
TARGET_DIR := ${EXE_DIR}
TGT_CXXFLAGS := ${CXXFLAGS}
TGT_LDFLAGS := ${LDFLAGS}
TGT_LDLIBS  := ${LDLIBS}
INCDIRS := ${INC_LIB_DIRS} ${INC_CXX_DIRS}
SUBMAKEFILES := $(addsuffix /sub.mk,$(CMP_DIRS))

# Exports
EXE_OUT := ${TARGET}
export EXE_OUT

# Check for CCACHE
CCACHE_EXISTS := $(shell command -v ccache)
ifneq ($(CCACHE_EXISTS),)
  ifeq ($(filter ccache,$(CXX)),)
    CXX := ccache $(CXX)
    export CXX
  endif

  ifeq ($(filter ccache,$(GCC)),)
    GCC := ccache $(GCC)
    export GCC
  endif
endif
