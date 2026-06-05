cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(rtl8733bs_bt_fw LANGUAGES C)

# Suppress cmake unused warning
set(ignore ${BUILD_DOC}	${BUILD_DOCS} ${BUILD_EXAMPLE} ${BUILD_EXAMPLES}
	${BUILD_SHARED_LIBS}${BUILD_TEST}${BUILD_TESTING}${BUILD_TESTS})


# Install
# install directories
if(NOT CMAKE_INSTALL_PREFIX)
	message(FATAL_ERROR "ERROR: CMAKE_INSTALL_PREFIX is not defined.")
endif()
include(GNUInstallDirs)

if(DEFINED CMAKE_INSTALL_FULL_LIBDIR)
	# Create directory first
	install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX}/")
	# Install test scripts and data
	install(FILES rtl8733bs_config DESTINATION "${CMAKE_INSTALL_PREFIX}/")
	install(FILES rtl8733bs_fw DESTINATION "${CMAKE_INSTALL_PREFIX}/")
endif() # CMAKE_INSTALL_FULL_LIBDIR

