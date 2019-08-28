add_executable(hui

hui.cpp
) 

target_include_directories(rtnet PRIVATE . ../../common)
target_include_directories(rtnet PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST})
target_link_libraries(rtnet ${CONAN_LIBS_BOOST})

target_compile_definitions(rtnet PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(rtnet PRIVATE ${SHAR_COMPILE_OPTIONS})