cmake_minimum_required(VERSION 3.12.2)

project (link11)


add_executable(l11 in.c link.c pass2.c sup.c)
add_executable(obint obint.c in.c sup.c)

install(TARGETS l11 obint
    COMPONENT linapp
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

