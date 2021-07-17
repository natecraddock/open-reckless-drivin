add_executable(open-reckless-drivin main.c initexit.c packs.c resource.c lzrw3-a.c sprites.c random.c interface.c quickdraw.c)

target_include_directories(open-reckless-drivin PUBLIC "include")
target_link_libraries(open-reckless-drivin m SDL2)
