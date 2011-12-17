
macro(TRANSLATION domain) #srcs
	find_package(Gettext REQUIRED)
# 	find_program(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
# 	if(NOT GETTEXT_MSGMERGE_EXECUTABLE)
# 		message(FATAL_ERROR "msgmerge not found")
# 	endif(NOT GETTEXT_MSGMERGE_EXECUTABLE)
# 	find_program(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
# 	if(NOT GETTEXT_MSGFMT_EXECUTABLE)
# 		message(FATAL_ERROR "msgfmt not found")
# 	endif(NOT GETTEXT_MSGFMT_EXECUTABLE)

	find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
	if(NOT GETTEXT_XGETTEXT_EXECUTABLE)
		message(FATAL_ERROR "xgettext not found")
	endif(NOT GETTEXT_XGETTEXT_EXECUTABLE)

	foreach(_arg ${ARGN})
		get_filename_component(_abs ${_arg} ABSOLUTE)
		set(_srcs ${_srcs} ${_abs})
	endforeach(_arg)
	set(_potfile "${CMAKE_SOURCE_DIR}/po/${domain}/${domain}.pot")
	add_custom_command(OUTPUT ${_potfile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
			-d ${domain} -s --keyword=_ --no-location --omit-header -o ${_potfile}
			${_srcs}
		DEPENDS ${_srcs}
	)
	file(GLOB _pofiles "${CMAKE_SOURCE_DIR}/po/${domain}/*.po")
# 	GETTEXT_CREATE_TRANSLATIONS(${CMAKE_BINARY_DIR}/po/${domain}.pot ALL ${_pofiles})
	foreach(_pofile ${_pofiles})
		get_filename_component(_lang ${_pofile} NAME_WE)
		add_custom_command( 
			OUTPUT ${CMAKE_BINARY_DIR}/share/locale/${_lang}/LC_MESSAGES/
			COMMAND ${CMAKE_COMMAND} -E make_directory  
				${CMAKE_BINARY_DIR}/share/locale/${_lang}/LC_MESSAGES/
		)
		set(_mofile "${CMAKE_BINARY_DIR}/share/locale/${_lang}/LC_MESSAGES/${domain}.mo")
		add_custom_command(
			OUTPUT ${_mofile}
			COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s ${_pofile} ${_potfile}
			COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_mofile} ${_pofile}
			DEPENDS ${_potfile} ${_pofile} ${CMAKE_BINARY_DIR}/share/locale/${_lang}/LC_MESSAGES/
		)
		install(FILES ${_mofile} DESTINATION share/locale/${_lang}/LC_MESSAGES)
		set(_mofiles ${_mofiles} ${_mofile})
	endforeach(_pofile)
	add_custom_target(${domain}_translation DEPENDS ${_mofiles})
	add_dependencies(${domain} ${domain}_translation)
endmacro(TRANSLATION)
