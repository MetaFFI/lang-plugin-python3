
#------------------------------- Python3 Language Support - Compiler Plugin ------------------------------------------
set(target_name "metaffi.compiler.python3")

include(${CMAKE_SCRIPTS_DIR}/MetaFFI.cmake)

# build
add_go_target(${target_name})

go_get(${target_name} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
go_build(${target_name} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# Copy to "METAFFI_HOME" to build IDL
copy_file_from_project_binary_to_metaffi_home(${target_name} ${target_name}${CMAKE_SHARED_LIBRARY_SUFFIX})

# install
install(FILES ${CMAKE_BINARY_DIR}/${target_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
		DESTINATION .)