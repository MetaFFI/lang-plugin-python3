# Load conan packages
find_or_install_package(Boost COMPONENTS filesystem)

# Collect plugin source files
collect_c_cpp_files(${CMAKE_CURRENT_LIST_DIR} xllr.python3)
remove_files_inside_path("${xllr.python3_src}" "deps")
set(xllr.python3_src ${NEW_FILES})

# Collect SDK cpython3 runtime_manager sources
collect_c_cpp_files("${metaffi_sdk_root}/runtime_manager/cpython3" sdk_cpython3_runtime_manager)

# Collect SDK cpython3 cdts_serializer sources
collect_c_cpp_files("${metaffi_sdk_root}/cdts_serializer/cpython3" sdk_cpython3_cdts_serializer)

# Combined SDK sources for cpython3
set(sdk_cpython3_src
	${sdk_src}
	${sdk_cpython3_runtime_manager_src}
	${sdk_cpython3_cdts_serializer_src}
)

# SDK include directories (add metaffi_sdk_root for <runtime_manager/cpython3/...> style includes)
set(sdk_cpython3_include
	${sdk_include_dir}
	${metaffi_sdk_root}
)

# Platform-specific link libraries
set(platform_libs "")
if(WIN32)
	# Version.lib is needed for GetFileVersionInfoA, VerQueryValueA, etc.
	# used in python_api_wrapper.cpp for detecting Python version from DLL
	set(platform_libs "Version.lib")
endif()

# Build shared library
c_cpp_shared_lib(xllr.python3
		"${xllr.python3_src};${sdk_cpython3_src}"
		"${sdk_cpython3_include};${Boost_INCLUDE_DIRS}"
		"Boost::filesystem;${platform_libs}"
		"./python3")

set(xllr.python3 xllr.python3 PARENT_SCOPE)
