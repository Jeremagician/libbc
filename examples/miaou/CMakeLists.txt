list(APPEND CHAT_SRC
	main.c
)

include_directories(${BC_INCLUDE_DIRS})

add_executable(miaou ${CHAT_SRC})
target_link_libraries(miaou bc)
