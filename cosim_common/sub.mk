# Sources definition
SRC_CXX_DIR = ./src

SRC_CXX = simd_pref_init.cpp \
		    simd_sys_scalar_run.cpp \
		    cosim_adapter.cpp

# build and link sc_main only for the top level of the project          
ifeq "${EXE_OUT}" "cosim"
	SRC_CXX += cosim_main.cpp
endif		  

# Specify variables for Boilermake
SOURCES := $(addprefix $(SRC_CXX_DIR)/,$(SRC_CXX))
