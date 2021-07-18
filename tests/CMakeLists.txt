function(test NAME DESCRIPTION)
  # any extra args are treated as additional sources to link to
  foreach(SRC ${ARGN})
    list(APPEND SOURCES "../src/${SRC}")
  endforeach()

  add_executable(${NAME} ${NAME}.c ${SOURCES})
  target_include_directories(${NAME} PUBLIC "../src/include")
  add_test(NAME ${DESCRIPTION} COMMAND ${NAME})
endfunction()

test(resource_test "resources" resource.c)
test(lzrw_test "lzrw-decompression" lzrw.c resource.c)
