add_executable(open-reckless-drivin
               gameframe.c
               gameinitexit.c
               initexit.c
               interface.c
               lzrw.c
               main.c
               packs.c
               quickdraw.c
               random.c
               resource.c
               sprites.c)
target_include_directories(open-reckless-drivin PUBLIC "include")
target_link_libraries(open-reckless-drivin m SDL2)
