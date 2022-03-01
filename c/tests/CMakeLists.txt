function(test NAME DESCRIPTION)
  # any extra args are treated as additional sources to link to
  foreach(SRC ${ARGN})
    list(APPEND SOURCES "../src/${SRC}")
  endforeach()

  add_executable(${NAME} ${NAME}.c ${SOURCES})
  target_include_directories(${NAME} PUBLIC "../src/include")

  # Set the working directory of the test to the source directory so
  # data files can be read easily
  add_test(NAME ${DESCRIPTION} COMMAND ${NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

test(resource_test "resource-loading" resource.c)
test(lzrw_test "lzrw-decompression" lzrw.c resource.c)
test(decryption_test "level-decryption" lzrw.c packs.c resource.c)
test(preference-test "preferences-reading" preferences.c)
