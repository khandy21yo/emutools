# - Find Zint
# Find the native Zint includes and library
#
#  ZINT_INCLUDE_DIR - where to find zint.h, etc.
#  ZINT_LIBRARIES   - List of libraries when using zint.
#  ZINT_FOUND       - True if zint found.

################### FIND ZINT ######################

IF (ZINT_INCLUDE_DIR)
  # Already in cache, be silent
  SET(ZINT_FIND_QUIETLY TRUE)
ENDIF (ZINT_INCLUDE_DIR)

FIND_PATH(ZINT_INCLUDE_DIR zint.h)

FIND_LIBRARY(ZINT_LIBRARY NAMES zint )

# handle the QUIETLY and REQUIRED arguments and set ZINT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Zint DEFAULT_MSG ZINT_LIBRARY ZINT_INCLUDE_DIR)

IF(ZINT_FOUND)
  SET( ZINT_LIBRARIES ${ZINT_LIBRARY} )
ELSE(ZINT_FOUND)
  SET( ZINT_LIBRARIES )
ENDIF(ZINT_FOUND)

MARK_AS_ADVANCED( ZINT_LIBRARY ZINT_INCLUDE_DIR )

# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)


