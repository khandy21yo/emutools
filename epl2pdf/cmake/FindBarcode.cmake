
#--------------------------------------------------------------------------------

# - Check for the presence of BARCODE
#
# The following variables are set when BARCODE is found:
#  BARCODE_FOUND      = Set to true, if all components of BARCODE have been found.
#  BARCODE_INCLUDE_DIRS   = Include path for the header files of BARCODE
#  BARCODE_LIBRARIES  = Link these to use BARCODE
#  BARCODE_LFLAGS     = Linker flags (optional)


INCLUDE(FindPackageHandleStandardArgs)
if (NOT BARCODE_FOUND)

  if (NOT BARCODE_ROOT_DIR)
    set (BARCODE_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
  endif (NOT BARCODE_ROOT_DIR)

  ##_____________________________________________________________________________
  ## Check for the header files

  find_path (BARCODE_INCLUDE_DIRS barcode.h
    HINTS ${BARCODE_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES include
    )

  ##_____________________________________________________________________________
  ## Check for the library

  find_library (BARCODE_LIBRARIES barcode
    HINTS ${BARCODE_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES lib
    )

  ##_____________________________________________________________________________
  ## Actions taken when all components have been found

  FIND_PACKAGE_HANDLE_STANDARD_ARGS (BARCODE DEFAULT_MSG BARCODE_LIBRARIES BARCODE_INCLUDE_DIRS)

  if (BARCODE_FOUND)
    if (NOT BARCODE_FIND_QUIETLY)
      message (STATUS "Found components for BARCODE")
      message (STATUS "BARCODE_ROOT_DIR  = ${BARCODE_ROOT_DIR}")
      message (STATUS "BARCODE_INCLUDE_DIRS  = ${BARCODE_INCLUDE_DIRS}")
      message (STATUS "BARCODE_LIBRARIES = ${BARCODE_LIBRARIES}")
    endif (NOT BARCODE_FIND_QUIETLY)
  else (BARCODE_FOUND)
    if (BARCODE_FIND_REQUIRED)
      message (FATAL_ERROR "Could not find BARCODE!")
    endif (BARCODE_FIND_REQUIRED)
  endif (BARCODE_FOUND)

  ##_____________________________________________________________________________
  ## Mark advanced variables

  mark_as_advanced (
    BARCODE_ROOT_DIR
    BARCODE_INCLUDE_DIRS
    BARCODE_LIBRARIES
    )

endif (NOT BARCODE_FOUND)
