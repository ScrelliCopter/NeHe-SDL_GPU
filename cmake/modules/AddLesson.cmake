function (add_lesson target)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;SHADERS;DATA")

	add_executable(${target} MACOSX_BUNDLE WIN32
		application.c application.h
		nehe.c nehe.h
		matrix.c matrix.h)
	set_property(TARGET ${target} PROPERTY C_STANDARD 99)

	target_compile_options(${target} PRIVATE
		$<$<C_COMPILER_ID:Clang,AppleClang>:-Weverything -Wno-declaration-after-statement -Wno-padded -Wno-switch-enum -Wno-cast-qual>
		$<$<C_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
		$<$<C_COMPILER_ID:MSVC>:/W4>)
	target_compile_definitions(${target} PRIVATE
		$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
	target_link_libraries(${target} SDL3::SDL3)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${target}>)
	endif()

	target_sources(${target} PRIVATE ${arg_SOURCES})

	foreach (shader IN LISTS arg_SHADERS)
		if (shader MATCHES "\\.metal$")
			set(path "${CMAKE_SOURCE_DIR}/src/shaders/${shader}")
		else()
			set(path "${CMAKE_SOURCE_DIR}/data/shaders/${shader}")
		endif()
		set_source_files_properties(${path} PROPERTIES
			HEADER_FILE_ONLY ON
			MACOSX_PACKAGE_LOCATION "Resources/Data/Shaders")
		target_sources(${target} PRIVATE "${path}")
	endforeach()
	foreach (resource IN LISTS arg_DATA)
		set(path "${CMAKE_SOURCE_DIR}/data/${resource}")
		set_source_files_properties(${path} PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources/Data")
		target_sources(${target} PRIVATE "${path}")
	endforeach()
	unset(path)
endfunction()
