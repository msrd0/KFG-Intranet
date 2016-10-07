include_directories(include)
add_executable(intranet
		include/defaultrequesthandler.h
		src/defaultrequesthandler.cpp
		src/main.cpp
	)
target_include_directories(intranet PUBLIC ${QtWebApp_INCLUDE_DIRS})
target_link_libraries(intranet ${QtWebApp_LIBRARIES} Qt5::Gui Qt5::Sql)
install(TARGETS intranet
        RUNTIME DESTINATION bin)
install(DIRECTORY conf   DESTINATION share/intranet)
install(DIRECTORY html   DESTINATION share/intranet)
install(DIRECTORY img    DESTINATION share/intranet)
install(DIRECTORY static DESTINATION share/intranet)
