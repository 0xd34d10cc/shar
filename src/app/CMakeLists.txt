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

    ${CMAKE_SOURCE_DIR}/res/resource.rc
    )

# Setting icon for macos is kinda tricky
if (APPLE) 
    set(MACOSX_BUNDLE_ICON_FILE mac.icns)
    set(myApp_ICON ${CMAKE_SOURCE_DIR}/res/mac.icns)
    set_source_files_properties(${myApp_ICON} PROPERTIES
           MACOSX_PACKAGE_LOCATION "Resources")
    
    add_executable(shar MACOSX_BUNDLE
                   ${SOURCE_FILES}
                   ${myApp_ICON}
                   )
else (APPLE)
    add_executable(shar WIN32
                   ${SOURCE_FILES}
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