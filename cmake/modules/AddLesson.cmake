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
		# Copy SDL3.dll to target build folder on Windows
		add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
			$<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${target}>)
	endif()

	target_sources(${target} PRIVATE ${arg_SOURCES})

	if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		# Ensure target Data & Data/Shaders folders exist
		add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E make_directory
			"$<TARGET_FILE_DIR:${target}>/Data/Shaders")
	endif()
	foreach (shader IN LISTS arg_SHADERS)
		if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			# Add compiled Metal shader libraries as bundle resources
			set(path "${CMAKE_SOURCE_DIR}/data/shaders/${shader}.metallib")
			set_source_files_properties(${path} PROPERTIES
				HEADER_FILE_ONLY ON
				MACOSX_PACKAGE_LOCATION "Resources/Data/Shaders")
			target_sources(${target} PRIVATE "${path}")
			unset(path)
		else()
			if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
				# Copy D3D12 (DXIL) shaders into target shaders folder
				add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
					"${CMAKE_SOURCE_DIR}/data/shaders/${shader}.vtx.dxb"
					"${CMAKE_SOURCE_DIR}/data/shaders/${shader}.pxl.dxb"
					"$<TARGET_FILE_DIR:${target}>/Data/Shaders")
			endif()
			# Copy Vulkan (SPIR-V) shaders into target shaders folder
			add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
				"${CMAKE_SOURCE_DIR}/data/shaders/${shader}.vtx.spv"
				"${CMAKE_SOURCE_DIR}/data/shaders/${shader}.frg.spv"
				"$<TARGET_FILE_DIR:${target}>/Data/Shaders")
		endif()
	endforeach()
	foreach (file IN LISTS arg_DATA)
		set(path "${CMAKE_SOURCE_DIR}/data/${file}")
		if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			# Add data file to bundle resources
			set_source_files_properties(${path} PROPERTIES
				MACOSX_PACKAGE_LOCATION "Resources/Data")
			target_sources(${target} PRIVATE "${path}")
		else()
			# Copy the data file into the target Data folder
			add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
				"${path}" "$<TARGET_FILE_DIR:${target}>/Data")
		endif()
		unset(path)
	endforeach()
endfunction()
