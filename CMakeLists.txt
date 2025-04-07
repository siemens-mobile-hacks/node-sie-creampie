cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 20)

project(creampie)

add_executable(creampie src/main.cpp ${LIB_SRC})
target_link_options(creampie PUBLIC
	-sWASM=1
	-sEXPORT_ES6=1
	-sMODULARIZE
	-sINVOKE_RUN=0
	-sSINGLE_FILE=0
	-sASSERTIONS=1
	-sEXIT_RUNTIME=1
	-sFILESYSTEM=0
	-sSTACK_OVERFLOW_CHECK=0
	-sALLOW_MEMORY_GROWTH
	-sEXPORTED_FUNCTIONS=['_malloc','_free']
	-sEXPORTED_RUNTIME_METHODS=['ccall']
	-fwasm-exceptions
	--bind
)
target_link_options(creampie PRIVATE
	--emit-tsd "$<TARGET_FILE_DIR:creampie>/creampie.d.ts"
)
target_compile_options(creampie PUBLIC -O3 -fwasm-exceptions)
