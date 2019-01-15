cmake_minimum_required (VERSION 2.6)


project (unmbasic)
set (unmbasic_VERSION_MAJOR 1)
set (unmbasic_VERSION_MINOR 0)
add_executable(unmbasic unmbasic.cc)
install (TARGETS unmbasic DESTINATION bin)

project (ungwbasic)
add_executable(ungwbasic ungwbasic.cc)
install (TARGETS ungwbasic DESTINATION bin)
set (ungwbasic_VERSION_MAJOR 1)
set (ungwbasic_VERSION_MINOR 0)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
     "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${unmbasic_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${unmbasic_VERSION_MINOR}")
include (CPack)

