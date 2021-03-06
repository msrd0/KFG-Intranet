cmake_minimum_required(VERSION 3.1)
project("KFG Intranet")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.2.0 REQUIRED COMPONENTS Core Gui Sql)
find_package(QtWebApp 1.7.0 REQUIRED COMPONENTS HttpServer TemplateEngine)
find_package(SPIS REQUIRED)

set(CMAKE_AUTOMOC ON)

add_custom_target(intranet.h sh ./CMakeVersionInfo.sh ${CMAKE_CURRENT_BINARY_DIR}
	              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				  SOURCES CMakeVersionInfo.sh)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB otherfiles conf/* html/* img/* static/*) # just for the ide
add_executable(intranet
		defaultrequesthandler.h
		defaultrequesthandler.cpp
		main.cpp
		db.spis
		${otherfiles}
	)
spis_compile(TARGET intranet FILES db.spis QTYPE)
add_dependencies(intranet intranet.h)
target_include_directories(intranet PUBLIC ${QtWebApp_INCLUDE_DIRS})
target_link_libraries(intranet ${QtWebApp_LIBRARIES} Qt5::Gui Qt5::Sql)
install(TARGETS intranet
        RUNTIME DESTINATION bin)
install(DIRECTORY conf   DESTINATION share/intranet)
install(DIRECTORY html   DESTINATION share/intranet)
install(DIRECTORY img    DESTINATION share/intranet)
install(DIRECTORY static DESTINATION share/intranet)
