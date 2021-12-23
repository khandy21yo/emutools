project(epl2pdf)
cmake_minimum_required(VERSION 3.1)

# The following line enables static analys of the code.
# It flags many things as warning that exit in the code, but are actually Ok.
# I'm not changing all the code to match the nnalysis. 
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

# popt isn't built into the standard cmake configuration, so I found a
# useable file on the internet an am pointing at that.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(POPT)
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
	podofo_barcode.cc
)
target_include_directories(
	epl2pdf
	PRIVATE ${POPT_INCLUDE_DIRS}
)
target_link_libraries(
	epl2pdf
	-lpodofo
	${POPT_LIBRARIES}
	${PoDoFo_LIBRARIES}
	zint
)

install(TARGETS  epl2pdf
    COMPONENT epl2pdf
    RUNTIME DESTINATION "/usr/local/bin/"
    LIBRARY DESTINATION "/usr/local/lib/"
    DESTINATION "/usr/local/bin"
)

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /dist
    /.*build.*
    /\\\\.DS_Store
)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "KH")
INCLUDE(CPack)
