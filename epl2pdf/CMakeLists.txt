project(epl2pdf)
cmake_minimum_required(VERSION 2.8)

# The following line enables static analys of the code.
# It flags many things as warning that exit in the code, but are actually Ok.
# I'm not changing all the code to match the nnalysis. 
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

# popt isn't built into the standard cmake configuration, so I found a
# useable file on the internet an am pointing at that.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PostgreSQL REQUIRED)	# Requires postgresql-server-dev-11
find_package(POPT)
find_package(Barcode)
#find_package(PoDoFo)
find_library(Zint zint)

add_executable(
	podofo_hello
	podofo_hello.cc
	podofo_barcode.cc
)
# The podofo library doesn't include useful things like a cmake config file
# so we have to handle this manually, which will probably break.
#target_include_directories(
#	podofo_hello
#	PRIVATE ${PoDoFo_INCLUDE_DIRS}
#)
target_link_libraries(
	podofo_hello
#	${PoDoFo_LIBRARIES}
	-lpodofo
	${Zint}
)

add_executable(
	epl2pdf
	epl2pdf.cc
)
target_include_directories(
	epl2pdf
	PRIVATE ${POPT_INCLUDE_DIRS}
)
target_link_libraries(
	epl2pdf
	-lpodofo
	${POPT_LIBRARIES}
)
