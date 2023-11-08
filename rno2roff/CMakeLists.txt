cmake_minimum_required(VERSION 3.5)
project(rno2roff)

# The following line enables static analys of the code.
# It flags many things as warning that exit in the code, but are actually Ok.
# I'm not changing all the code to match the nnalysis. 
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

# popt isn't built into the standard cmake configuration, so I found a
# useable file on the internet an am pointing at that.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(
	rno2roff
	rno2roff.cc
)
target_link_libraries(
	rno2roff
)

install(TARGETS  rno2roff
    COMPONENT rno2roff
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
