# Copyright (c) 2017 The Bitcoin developers
# Copyright (c) 2019 Bitcoin Association
# Distributed under the Open BSV software license, see the accompanying file LICENSE.

cmake_minimum_required(VERSION 3.5)
project(BitcoinSV)

set(CMAKE_CXX_STANDARD 17)

# Default visibility is hidden on all targets.
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)


# Ensure that WINDRES_PREPROC is enabled when using windres.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND CMAKE_RC_FLAGS "-DWINDRES_PREPROC")
endif()

# Enable warning
include(AddCompilerFlags)

add_c_compiler_flag(-Wnested-externs -Wstrict-prototypes)
add_compiler_flag(
	-Wall
	-Wextra
	-Wformat
	-Wvla
	-Wformat-security
	-Wcast-align
	-Wunused-parameter
)

option(EXTRA_WARNINGS "Enable extra warnings" OFF)
if(EXTRA_WARNINGS)
	add_compiler_flag(-Wshadow)
	add_cxx_compiler_flag(-Wsuggest-override)
else()
	add_compiler_flag(-Wno-unused-parameter)
endif()

if(MSVC)
	# Do not define min/max macros - we use std::min/max
	add_compile_definitions(NOMINMAX)
	# Perform parallel builds:
	add_compile_options("/MP")

	# Fix Cmake's default compiler options - see also https://gitlab.kitware.com/cmake/community/wikis/FAQ#dynamic-replace
	# - remove default NDEBUG compile definition to solve #error "Bitcoin cannot be compiled without assertions"
	# - use static runtime instead of dynamic runtime to match the external libraries  
	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
		
	if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")    
		endif()

		if(${flag_var} MATCHES "/DNDEBUG")
			string(REGEX REPLACE "/DNDEBUG" "" ${flag_var} "${${flag_var}}")
		endif()

	endforeach()

endif()

# libtool style configure
add_subdirectory(config)

# libraries
add_subdirectory(crypto)
add_subdirectory(leveldb)
add_subdirectory(secp256k1)

# Because the Bitcoin ABc source code is disorganised, we
# end up with a bunch of libraries without any aparent
# cohesive structure. This is inherited from Bitcoin Core
# and reflecting this.
# TODO: Improve the structure once cmake is rocking.

# Various completely unrelated features shared by all executables.
add_library(util
	chainparamsbase.cpp
	clientversion.cpp
	compat/glibc_sanity.cpp
	compat/glibcxx_sanity.cpp
	compat/strnlen.cpp
	fs.cpp
	logging.cpp
	random.cpp
	support/cleanse.cpp
	support/lockedpool.cpp
	sync.cpp
	threadinterrupt.cpp
	uint256.cpp
    arith_uint256.cpp
	util.cpp
	utilstrencodings.cpp
	utiltime.cpp
)

if(ENABLE_PROD_BUILD)
    target_compile_definitions(util PUBLIC IS_PRODUCTION_BUILD)
endif()
target_compile_definitions(util PUBLIC HAVE_CONFIG_H)
target_include_directories(util
	PUBLIC
		.
		# To access the config.
		${CMAKE_CURRENT_BINARY_DIR}
	PRIVATE
		${OPENSSL_INCLUDE_DIR}
)

# Target specific configs
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_THREADAPI win32)

	find_package(SHLWAPI REQUIRED)
	target_link_libraries(util ${SHLWAPI_LIBRARY})
	target_include_directories(util PUBLIC ${SHLWAPI_INCLUDE_DIR})

	find_library(WS2_32_LIBRARY NAMES ws2_32)
	target_link_libraries(util ${WS2_32_LIBRARY})
	
	target_compile_definitions(util PUBLIC BOOST_THREAD_USE_LIB)
endif()

# Boost packages
set(BOOST_PACKAGES_REQUIRED chrono filesystem program_options thread)

function(prepend var prefix)
	set(listVar "")
	foreach(f ${ARGN})
		list(APPEND listVar "${prefix}${f}")
	endforeach(f)
	set(${var} "${listVar}" PARENT_SCOPE)
endfunction(prepend)

prepend(BOOST_LIBRARIES "Boost::" ${BOOST_PACKAGES_REQUIRED})

find_package(Boost 1.58 REQUIRED ${BOOST_PACKAGES_REQUIRED})
target_link_libraries(util univalue crypto ${BOOST_LIBRARIES})

# Make sure boost uses std::atomic (it doesn't before 1.63)
target_compile_definitions(util PUBLIC BOOST_SP_USE_STD_ATOMIC BOOST_AC_USE_STD_ATOMIC)

# More completely unrelated features shared by all executables.
# Because nothing says this is different from util than "common"
add_library(common
            hash.cpp
            chain.cpp
            primitives/transaction.cpp
            uint256.cpp
            utilstrencodings.cpp
            primitives/block.cpp
            amount.cpp
            script/script.cpp
            )

target_link_libraries(common util secp256k1)

# Bitcoin server facilities
add_library(server
	txdb.cpp
    )
# This require libevent
find_package(Event REQUIRED)

target_include_directories(server
        PRIVATE
		leveldb/helpers/memenv
		${EVENT_INCLUDE_DIR}
)

target_link_libraries(server
	${EVENT_LIBRARY}
    common
	leveldb
	memenv
)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(server ${EVENT_PTHREAD_LIBRARY})
else()
  # New mining API uses boost uuid library which requires bcrypt.lib on Windows
  find_library(BCRYPT_LIBRARY NAMES bcrypt)
  target_link_libraries(server ${BCRYPT_LIBRARY})
endif()

add_executable(blockread blockread.cpp)
target_link_libraries(blockread server)

