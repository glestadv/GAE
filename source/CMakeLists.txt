# source directory

add_subdirectory(shared_lib)
add_subdirectory(game)
add_subdirectory(blendnorm)
if(wxWidgets_FOUND)
	include(${wxWidgets_USE_FILE})

	add_subdirectory(map_editor)
	add_subdirectory(g3d_viewer)
	add_subdirectory(configurator)

	if(UNIX AND NOT APPLE)
		# define .g3d and .gbm file types to associate default programs
		install(FILES gae-mime.xml DESTINATION share/mime/packages)
	endif(UNIX AND NOT APPLE)
else(wxWidgets_FOUND)
	message(STATUS "disabled map editor, g3d viewer and configurator")
endif(wxWidgets_FOUND)

if(CPPUNIT_FOUND)
	add_subdirectory(test)
else(CPPUNIT_FOUND)
	message(STATUS "disabled testsuite")
endif(CPPUNIT_FOUND)

if(LIBXML2_FOUND)
	add_subdirectory(glexemel)
else(LIBXML2_FOUND)
	message(STATUS "disabled glexemel")
endif(LIBXML2_FOUND)
