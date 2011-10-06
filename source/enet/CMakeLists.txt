# enet static lib

set(enet_incs "${CMAKE_CURRENT_SOURCE_DIR}/include")
file(GLOB enet_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "sources/*.c" "include/enet/*.h")

# needed to get added to game project
set(enet_incs ${enet_incs} PARENT_SCOPE)

include_directories(${enet_incs})

add_library(enet_lib STATIC ${enet_srcs})

#if(WIN32)
#	target_link_libraries(enet_lib 
#		)
#endif(WIN32)

if (MSVC)
	# set linker flags for Release to use link time code gen.
	set_target_properties(enet_lib PROPERTIES LINK_FLAGS_RELEASE "/LTCG")
	set_target_properties(enet_lib PROPERTIES COMPILE_FLAGS "/DUSE_PCH=0")
endif(MSVC)
