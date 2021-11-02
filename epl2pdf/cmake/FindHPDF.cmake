
#--------------------------------------------------------------------------------

# - Check for the presence of HPDF
#
# The following variables are set when HPDF is found:
#  HPDF_FOUND      = Set to true, if all components of HPDF have been found.
#  HPDF_INCLUDE_DIRS   = Include path for the header files of HPDF
#  HPDF_LIBRARIES  = Link these to use HPDF
#  HPDF_LFLAGS     = Linker flags (optional)


INCLUDE(FindPackageHandleStandardArgs)
if (NOT HPDF_FOUND)

  if (NOT HPDF_ROOT_DIR)
    set (HPDF_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
  endif (NOT HPDF_ROOT_DIR)

  ##_____________________________________________________________________________
  ## Check for the header files

  find_path (HPDF_INCLUDE_DIRS hpdf.h
    HINTS ${HPDF_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES include
    )

  ##_____________________________________________________________________________
  ## Check for the library

  find_library (HPDF_LIBRARIES hpdf
    HINTS ${HPDF_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES lib
    )

  ##_____________________________________________________________________________
  ## Actions taken when all components have been found

  FIND_PACKAGE_HANDLE_STANDARD_ARGS (HPDF DEFAULT_MSG HPDF_LIBRARIES HPDF_INCLUDE_DIRS)

  if (HPDF_FOUND)
    if (NOT HPDF_FIND_QUIETLY)
      message (STATUS "Found components for HPDF")
      message (STATUS "HPDF_ROOT_DIR  = ${HPDF_ROOT_DIR}")
      message (STATUS "HPDF_INCLUDE_DIRS  = ${HPDF_INCLUDE_DIRS}")
      message (STATUS "HPDF_LIBRARIES = ${HPDF_LIBRARIES}")
    endif (NOT HPDF_FIND_QUIETLY)
  else (HPDF_FOUND)
    if (HPDF_FIND_REQUIRED)
      message (FATAL_ERROR "Could not find HPDF!")
    endif (HPDF_FIND_REQUIRED)
  endif (HPDF_FOUND)

  ##_____________________________________________________________________________
  ## Mark advanced variables

  mark_as_advanced (
    HPDF_ROOT_DIR
    HPDF_INCLUDE_DIRS
    HPDF_LIBRARIES
    )

endif (NOT HPDF_FOUND)
