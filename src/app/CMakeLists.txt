set(SOURCE_FILES
    main.cpp
    runner.hpp
    runner.cpp
    app.hpp
    app.cpp
    broadcast.hpp
    broadcast.cpp
    view.hpp
    view.cpp
    )


if (APPLE)
    set(SHAR_ICON ${CMAKE_SOURCE_DIR}/res/mac.icns)
    set_source_files_properties(${SHAR_ICON} PROPERTIES
                                MACOSX_PACKAGE_LOCATION "Resources"
                               )
elseif (WIN32)
    set(SHAR_ICON ${CMAKE_SOURCE_DIR}/res/resource.rc)
endif(APPLE)

add_executable(shar
               ${SOURCE_FILES}
               ${SHAR_ICON}
               )

if (APPLE) 
    set_target_properties(shar PROPERTIES
                          MACOSX_BUNDLE TRUE
                         )
elseif (WIN32)
    set_target_properties(shar PROPERTIES
                          WIN32_EXECUTABLE TRUE
                         )    
endif (APPLE)

                   

target_include_directories(shar
                           PRIVATE ..
                           PRIVATE ../common
                           )

target_link_libraries(shar
                      PRIVATE capture
                      PRIVATE codec
                      PRIVATE net
                      PRIVATE ui
                      PRIVATE common
                      )

target_compile_definitions(shar PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(shar PRIVATE ${SHAR_COMPILE_OPTIONS})

IF (NOT WIN32)
    message("-- Enabling debug info")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-omit-frame-pointer")

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      message("-- Enabling LTO")
      set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif ()
endif ()