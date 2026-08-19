cmake_minimum_required(VERSION 3.22)

include(FetchContent)

# Download CMock (w/ Unity included) as a dependency
FetchContent_Declare(
	cmock
	GIT_REPOSITORY https://github.com/throwtheswitch/cmock.git
	GIT_TAG v2.7.0
)
FetchContent_MakeAvailable(cmock)

set(CMOCK_DIR ${cmock_SOURCE_DIR})
set(UNITY_DIR ${CMOCK_DIR}/vendor/unity)

# Build Unity as a static library
add_library(unity STATIC
	${UNITY_DIR}/src/unity.c
)
target_include_directories(unity PUBLIC
	${UNITY_DIR}/src
)

# Build CMock as a static library
add_library(cmock_lib STATIC
	${CMOCK_DIR}/src/cmock.c
)
target_include_directories(cmock_lib PUBLIC
	${CMOCK_DIR}/src
)
target_link_libraries(cmock_lib PUBLIC unity)


# CMock's mock generator is a Ruby script, so Ruby must be installed.
find_program(RUBY_EXECUTABLE ruby REQUIRED)
function(add_cmock_test)
	# ---- Parse the function arguments ----
	cmake_parse_arguments(
		ARG                              # Prefix for parsed variables
		""                               # No boolean flags
		"NAME"                           # Single-value args: NAME
		"SOURCES;MOCKS;INCLUDES;COMPILE_DEFS"  # Multi-value args
		${ARGN}                          # The actual arguments passed to the function
	)
	# After parsing, we can access:
	#   ARG_NAME         = the test name (e.g. "test_commands")
	#   ARG_SOURCES      = list of .c files to compile
	#   ARG_MOCKS        = list of headers to generate mocks from
	#   ARG_INCLUDES     = list of directories to add to the include path
	#   ARG_COMPILE_DEFS = list of -D defines private to this test target

	# ---- Generate mocks ----
	set(MOCK_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/mocks/${ARG_NAME})
	file(MAKE_DIRECTORY ${MOCK_OUTPUT_DIR})

	set(MOCK_SOURCES "")
	foreach (MOCK_HEADER ${ARG_MOCKS})
		get_filename_component(MOCK_BASE ${MOCK_HEADER} NAME_WE)

		set(MOCK_C ${MOCK_OUTPUT_DIR}/mock_${MOCK_BASE}.c)
		set(MOCK_H ${MOCK_OUTPUT_DIR}/mock_${MOCK_BASE}.h)

		add_custom_command(
			OUTPUT ${MOCK_C} ${MOCK_H}
			COMMAND ${RUBY_EXECUTABLE}
			${CMOCK_DIR}/lib/cmock.rb
			-o${CMAKE_CURRENT_SOURCE_DIR}/cmock_config.yml
			${MOCK_HEADER}
			DEPENDS ${MOCK_HEADER} cmock_config.yml
			WORKING_DIRECTORY ${MOCK_OUTPUT_DIR}
			COMMENT "CMock: Generating mock for ${MOCK_HEADER}"
		)

		list(APPEND MOCK_SOURCES ${MOCK_C})
	endforeach ()

	# ---- Create the test executable ----
	add_executable(${ARG_NAME}
		${ARG_SOURCES}
		${MOCK_SOURCES}
	)

	target_include_directories(${ARG_NAME} PRIVATE
		${MOCK_OUTPUT_DIR}
		${ARG_INCLUDES}
	)

	target_link_libraries(${ARG_NAME} PRIVATE
		unity
		cmock_lib
	)

	# ---- Optional per-target compile definitions (e.g. BUILD_TESTING) ----
	# Used to expose #ifdef-guarded SUT accessor blocks to a single test
	# target without leaking the define into the production firmware build.
	if (ARG_COMPILE_DEFS)
		target_compile_definitions(${ARG_NAME} PRIVATE ${ARG_COMPILE_DEFS})
	endif ()

	# ---- Register with CTest ----
	add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
endfunction()


# Adding middleware unit tests
# Test 1 - BSP test
add_cmock_test(
	NAME test_middleware_bsp
	SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/middleware/test_bsp.c
		${CMAKE_SOURCE_DIR}/winc1500/bsp/source/nm_bsp_stm32f401xe.c
	MOCKS
		${CMAKE_CURRENT_SOURCE_DIR}/middleware/dummy_headers/conf_winc.h
		${CMAKE_CURRENT_SOURCE_DIR}/middleware/dummy_headers/stm32_hal.h
		${CMAKE_CURRENT_SOURCE_DIR}/middleware/dummy_headers/mw_port.h
	INCLUDES
    	${CMAKE_CURRENT_SOURCE_DIR}/middleware/dummy_headers
		${CMAKE_SOURCE_DIR}/winc1500
		${CMAKE_SOURCE_DIR}/winc1500/bsp/include
		${CMAKE_SOURCE_DIR}/winc1500/common/include
	COMPILE_DEFS
		BUILD_TESTING
)
