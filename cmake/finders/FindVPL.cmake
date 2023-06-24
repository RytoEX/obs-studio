#[=======================================================================[.rst
FindVPL
-------

FindModule for VPL and associated headers

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``VPL::VPL``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``VPL_FOUND``
  True, if headers were found.
``VPL_VERSION``
  Detected version of found VPL headers.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``VPL_INCLUDE_DIR``
  Directory containing ``mfxdispatcher.h``.
``VPL_LIB``
  Path to the library component of VPL.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_VPL vpl)
endif()

find_path(
  VPL_INCLUDE_DIR
  NAMES vpl/mfxstructures.h
  HINTS ${_VPL_INCLUDE_DIRS} ${_VPL_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  DOC "VPL include directory")

find_library(
  VPL_LIB
  NAMES ${_VPL_LIBRARIES} ${_VPL_LIBRARIES} vpl
  HINTS ${_VPL_LIBRARY_DIRS} ${_VPL_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  DOC "VPL library location")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  VPL
  REQUIRED_VARS VPL_LIB VPL_INCLUDE_DIR
  VERSION_VAR VPL_VERSION REASON_FAILURE_MESSAGE "${VPL_ERROR_REASON}")
mark_as_advanced(VPL_INCLUDE_DIR VPL_LIB)
unset(VPL_ERROR_REASON)

if(EXISTS "${VPL_INCLUDE_DIR}/vpl/mfxdefs.h")
  file(STRINGS "${VPL_INCLUDE_DIR}/vpl/mfxdefs.h" _version_string REGEX "^.*VERSION_(MAJOR|MINOR)[ \t]+[0-9]+[ \t]*$")

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")

  set(VPL_VERSION "${_version_major}.${_version_minor}")
  unset(_version_major)
  unset(_version_minor)
else()
  if(NOT VPL_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find VPL version.")
  endif()
  set(VPL_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(VPL_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(VPL_ERROR_REASON "Ensure vpl headers are available in local library paths.")
endif()

if(VPL_FOUND)
  set(VPL_INCLUDE_DIRS ${VPL_INCLUDE_DIR})
  set(VPL_LIBRARIES ${VPL_LIB})

  if(NOT TARGET VPL::VPL)
    if(IS_ABSOLUTE "${VPL_LIBRARIES}")
      add_library(VPL::VPL UNKNOWN IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LOCATION "${VPL_LIBRARIES}")
    else()
      add_library(VPL::VPL INTERFACE IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LIBNAME "${VPL_LIBRARIES}")
    endif()

    set_target_properties(VPL::VPL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VPL_INCLUDE_DIRS}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  VPL PROPERTIES
  URL "https://github.com/oneapi-src/oneVPL"
  DESCRIPTION
    "Intel® oneAPI Video Processing Library (oneVPL) supports AI visual inference, media delivery, cloud gaming, and virtual desktop infrastructure use cases by providing access to hardware accelerated video decode, encode, and frame processing capabilities on Intel® GPUs."
)
