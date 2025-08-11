cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 20)

project(creampie)

add_executable(creampie src/main.cpp ${LIB_SRC})
target_link_options(creampie PRIVATE
	-sWASM=1
	-sSTRICT=1
	-sEXPORT_ES6=1
	-sMODULARIZE
	-sINVOKE_RUN=0
	-sSINGLE_FILE=0
	-sASSERTIONS=0
	-sEXIT_RUNTIME=1
	-sFILESYSTEM=0
	-sDYNAMIC_EXECUTION=0
	-sSTACK_OVERFLOW_CHECK=0
	-sALLOW_MEMORY_GROWTH
	-sENVIRONMENT=node,web,worker
	-sEXPORTED_FUNCTIONS=['_malloc','_free']
	-sEXPORTED_RUNTIME_METHODS=['ccall','HEAPU8']
	-sMALLOC=emmalloc
	-fwasm-exceptions
	--closure 1
	--bind
)
target_link_options(creampie PRIVATE
	--emit-tsd "$<TARGET_FILE_DIR:creampie>/creampie.d.ts"
)
target_compile_options(creampie PRIVATE -Oz -fwasm-exceptions)
