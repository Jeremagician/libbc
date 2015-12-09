include_directories(${BC_INCLUDE_DIRS})

list(APPEND BC_LIB_SRC
  broadcast.c
  event.c
  network.c
)

add_library(bc ${BC_LIB_SRC})
