cmake_minimum_required(VERSION 2.8.11)
project("KFG Intranet")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.2.0 REQUIRED COMPONENTS Core Gui Sql)
find_package(QtWebApp REQUIRED COMPONENTS HttpServer TemplateEngine)
find_package(QSL REQUIRED)

set(CMAKE_AUTOMOC ON)

execute_process(COMMAND sh ./CMakeVersionInfo.sh ${CMAKE_CURRENT_BINARY_DIR}
	            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(Intranet)

install(FILES config.ini
	    DESTINATION /etc/intranet)
