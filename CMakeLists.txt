cmake_minimum_required(VERSION 2.8.11)
project("KFG Intranet")

find_package(Qt5 5.2.0 REQUIRED COMPONENTS Core Gui Sql)
find_package(QtWebApp REQUIRED COMPONENTS HttpServer TemplateEngine)

set(CMAKE_AUTOMOC ON)

message("dirs: ${QtWebApp_INCLUDE_DIRS}")
message("libs: ${QtWebApp_LIBRARIES}")

add_subdirectory(Intranet)

install(FILES config.ini
	    DESTINATION /etc/intranet)
