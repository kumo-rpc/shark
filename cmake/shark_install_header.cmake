# install header files
install(DIRECTORY ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.inc"
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.shark"
)

install(FILES shark_cc_generate.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

