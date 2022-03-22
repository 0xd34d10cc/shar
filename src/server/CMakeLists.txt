add_executable(server
  main.cpp
  server.hpp
  server.cpp
)

target_include_directories(server
  PRIVATE ..
  PRIVATE ../common
)

target_link_libraries(server
  PRIVATE common
  PRIVATE net
  PRIVATE proto
)

target_compile_definitions(server PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(server PRIVATE ${SHAR_COMPILE_OPTIONS})