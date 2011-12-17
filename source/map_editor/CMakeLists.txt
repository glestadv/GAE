# glest map editor

file(GLOB map_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
if(WIN32)
	list(APPEND map_srcs "resources.rc")
endif(WIN32)

# find_package(Gettext REQUIRED)
# find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
# if(NOT GETTEXT_XGETTEXT_EXECUTABLE ) 
# 	message(FATAL_ERROR "xgettext not found") 
# endif(NOT GETTEXT_XGETTEXT_EXECUTABLE )
# add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gae_mapeditor.pot
# 	COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
# 		-d gae_mapeditor -s --keyword=_ -o ${CMAKE_CURRENT_BINARY_DIR}/gae_mapeditor.pot
# 		${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
# 	DEPENDS main.cpp
# )
# file(GLOB pofiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "po/*.po")
# gettext_create_translations(${CMAKE_CURRENT_BINARY_DIR}/gae_mapeditor.pot ALL ${pofiles})

include_directories(${glestshared_incs})
add_executable(gae_mapeditor ${map_srcs})
target_link_libraries(gae_mapeditor shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(gae_mapeditor PROPERTIES WIN32_EXECUTABLE "true")

include(${CMAKE_SOURCE_DIR}/cmake/translation.cmake)
translation(gae_mapeditor main.cpp)

install(TARGETS gae_mapeditor DESTINATION bin)
if(UNIX AND NOT APPLE)
	install(FILES gae_mapeditor.png DESTINATION share/pixmaps)
	install(FILES gae_mapeditor.desktop DESTINATION share/applications)
endif(UNIX AND NOT APPLE)

if(CPPCHECK_FOUND)
	add_cppcheck(gae_mapeditor)
endif(CPPCHECK_FOUND)
