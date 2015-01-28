# - Find SLAM6D
# Find the SLAM6D includes and library
#
#  SLAM6D_INCLUDE_DIR - Where to find SLAM6D includes
#  SLAM6D_LIBRARIES   - List of libraries when using SLAM6D
#  SLAM6D_FOUND       - True if SLAM6D was found

IF(SLAM6D_INCLUDE_DIR)
  SET(SLAM6D_FIND_QUIETLY TRUE)
ENDIF(SLAM6D_INCLUDE_DIR)

FIND_PATH(SLAM6D_INCLUDE_DIR "slam6d/scan.h"
  PATHS
  $ENV{3DTK_HOME}/include
  $ENV{EXTERNLIBS}/3dtk/include
  ~/Library/Frameworks/include
  /Library/Frameworks/include
  /usr/local/include
  /usr/include
  /sw/include # Fink
  /opt/local/include # DarwinPorts
  /opt/csw/include # Blastwave
  /opt/include
  DOC "SLAM6D - Headers"
)

SET(SLAM6D_NAMES scanlib32)
SET(SLAM6D_DBG_NAMES scanlibd32)
SET(ANN_NAMES ANN32)
SET(ANN_DBG_NAMES ANNd32)

FIND_LIBRARY(SLAM6D_LIBRARY NAMES ${SLAM6D_NAMES}
  PATHS
  $ENV{3DTK_HOME}
  $ENV{EXTERNLIBS}/3dtk
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  PATH_SUFFIXES lib lib64
  DOC "SLAM6D - Library"
)

FIND_LIBRARY(ANN_LIBRARY NAMES ${ANN_NAMES}
  PATHS
  $ENV{3DTK_HOME}
  $ENV{EXTERNLIBS}/3dtk
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  PATH_SUFFIXES lib lib64
  DOC "SLAM6D - Library"
)

INCLUDE(FindPackageHandleStandardArgs)

IF(MSVC)
  # VisualStudio needs a debug version
  FIND_LIBRARY(SLAM6D_LIBRARY_DEBUG NAMES ${SLAM6D_DBG_NAMES}
    PATHS
    $ENV{3DTK_HOME}/lib
    $ENV{EXTERNLIBS}/3dtk/lib
    DOC "SLAM6D - Library (Debug)"
  )
  FIND_LIBRARY(ANN_LIBRARY_DEBUG NAMES ${ANN_DBG_NAMES}
    PATHS
    $ENV{3DTK_HOME}/lib
    $ENV{EXTERNLIBS}/3dtk/lib
    DOC "SLAM6D - Library (Debug)"
  )
  
  IF(SLAM6D_LIBRARY_DEBUG AND SLAM6D_LIBRARY)
    SET(SLAM6D_LIBRARIES optimized ${SLAM6D_LIBRARY} debug ${SLAM6D_LIBRARY_DEBUG} optimized ${ANN_LIBRARY} debug ${ANN_LIBRARY_DEBUG})
  ENDIF(SLAM6D_LIBRARY_DEBUG AND SLAM6D_LIBRARY)

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(SLAM6D DEFAULT_MSG SLAM6D_LIBRARY SLAM6D_LIBRARY_DEBUG ANN_LIBRARY ANN_LIBRARY_DEBUG SLAM6D_INCLUDE_DIR)

  MARK_AS_ADVANCED(SLAM6D_LIBRARY SLAM6D_LIBRARY_DEBUG ANN_LIBRARY ANN_LIBRARY_DEBUG SLAM6D_INCLUDE_DIR)
  
ELSE(MSVC)
  # rest of the world
  SET(SLAM6D_LIBRARIES ${SLAM6D_LIBRARY} ${ANN_LIBRARY})

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(SLAM6D DEFAULT_MSG SLAM6D_LIBRARY SLAM6D_INCLUDE_DIR)
  
  MARK_AS_ADVANCED(SLAM6D_LIBRARY SLAM6D_INCLUDE_DIR)
  
ENDIF(MSVC)

IF(SLAM6D_FOUND)
  SET(SLAM6D_INCLUDE_DIRS ${SLAM6D_INCLUDE_DIR})
ENDIF(SLAM6D_FOUND)