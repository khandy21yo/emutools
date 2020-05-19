cmake_minimum_required(VERSION 3.12.2)

project (link11 VERSION 0.2
	DESCRIPTION "PDP-11 linker"
	LANGUAGES C CXX)


add_executable(l11 in.c link.c pass2.c sup.c)
add_executable(obint obint.c in.c sup.c)
add_executable(link11 link11main.cc link11read.cc link11link.cc link11util.cc)

install(TARGETS l11 obint
    COMPONENT linapp
    RUNTIME DESTINATION "/usr/local/bin/"
    LIBRARY DESTINATION "/usr/local/lib/"
    DESTINATION "/usr/local/bin"
)

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

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

