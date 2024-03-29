cmake_minimum_required(VERSION 3.16.3)

if("$ENV{METAFFI_HOME}" STREQUAL "")
	message(FATAL_ERROR "METAFFI_HOME is missing - MetaFFI must be installed")
endif()

# policies
cmake_policy(SET CMP0110 NEW) # Allow arbitrary names in CTest names

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_STANDARD 20)
if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-DEBUG) # add -DEBUG to "Debug" builds
	set(CMAKE_DEBUG_POSTFIX "")
endif()

# Add SDK
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/plugin-sdk/)
set(METAFFI_SDK ${CMAKE_CURRENT_LIST_DIR}/plugin-sdk)
set(CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_LIST_DIR}/plugin-sdk/cmake)

# Load hunter package manager
load_hunter_pm(${CMAKE_SCRIPTS_DIR}) # load package manager

# include scripts required for project
include(${CMAKE_SCRIPTS_DIR}/InstallUtils.cmake)

#-----------------------------------------------------------------------------------------------

project(lang-plugin-python3 VERSION 0.0.1)

# Add required packages
hunter_add_package(Catch)
add_ctest_and_unitest_libs()

# set RPATH
set(CMAKE_MACOSX_RPATH 1) # tell MacOS RPATH is in use
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/lib")
list(APPEND CMAKE_BUILD_RPATH "$ORIGIN;$ORIGIN/lib")

# add targets
#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/compiler/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/runtime/)
#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/idl/)

# cmake install (not cpack)
set(CMAKE_INSTALL_PREFIX [[$ENV{METAFFI_HOME}]])
set(CMAKE_SKIP_INSTALL_RPATH ON)

# package notes
set(INSTALL_NOTES "Standard Python3 (e.g. CPython) must be installed to use plugin.
In case of 'Could not find platform independent libraries', set PYTHONHOME environment variable to install path")

file(WRITE ${PROJECT_BINARY_DIR}/notes.txt ${INSTALL_NOTES})

#-----------------------------------------------------------------------------------------------

install(CODE "set(CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_LIST_DIR}/plugin-sdk/cmake)")
install(CODE "set(PACKAGE_ROOT ${PROJECT_BINARY_DIR})")
install(CODE "set(dylib_ext ${CMAKE_SHARED_LIBRARY_SUFFIX})")
install(CODE
		[[
		include(${CMAKE_SCRIPTS_DIR}/MetaFFI.cmake)

		list(APPEND package_files notes.txt)
		list(APPEND package_files metaffi.idl.py${dylib_ext})
		list(APPEND package_files py_extractor.py)
		list(APPEND package_files py_extractor_MetaFFIGuest.py)
		list(APPEND package_files metaffi.compiler.python3${dylib_ext})
		list(APPEND package_files xllr.python311${dylib_ext})
		metaffi_pack("${PACKAGE_ROOT}" "${package_files}")
]])