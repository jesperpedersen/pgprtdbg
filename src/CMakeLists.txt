#
# Add files for libpgagroal
#
FILE(GLOB SOURCE_FILES "libpgprtdbg/*.c")
FILE(GLOB HEADER_FILES "include/*.h")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

#
# Include directories
#
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LIBEV_INCLUDE_DIRS}
)

#
# Library directories
#
link_libraries(
  ${LIBEV_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

#
# Compile options
#
add_compile_options(-g)
add_compile_options(-Wall)
add_compile_options(-std=c17)
add_compile_options(-D_POSIX_C_SOURCE=200809L)
add_compile_options(-D__USE_ISOC11)
add_compile_options(-D_GNU_SOURCE)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer -DDEBUG")

if (CMAKE_BUILD_TYPE MATCHES Release)
  add_compile_options(-O2)

  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-march=native)
    add_compile_options(-mtune=native)
    add_compile_options(-flto)
  endif()
endif (CMAKE_BUILD_TYPE MATCHES Release)

#
# Build libpgprtdbg
#
add_library(pgprtdbg SHARED ${SOURCES})
set_target_properties(pgprtdbg PROPERTIES LINKER_LANGUAGE C VERSION ${VERSION_STRING}
                               SOVERSION ${VERSION_MAJOR})
target_link_libraries(pgprtdbg PUBLIC)

install(TARGETS pgprtdbg DESTINATION ${CMAKE_INSTALL_LIBDIR}/)

#
# Build pgprtdbg
#
add_executable(pgprtdbg-bin main.c ${RESOURCE_OBJECT})
set_target_properties(pgprtdbg-bin PROPERTIES LINKER_LANGUAGE C OUTPUT_NAME pgprtdbg)
target_link_libraries(pgprtdbg-bin pgprtdbg)

install(TARGETS pgprtdbg-bin DESTINATION ${CMAKE_INSTALL_BINDIR})

#
# Install configuration and documentation
#
install(FILES ../doc/etc/pgprtdbg.conf DESTINATION etc)

install(DIRECTORY ../doc/ DESTINATION share/doc/pgprtdbg FILES_MATCHING PATTERN "*")
