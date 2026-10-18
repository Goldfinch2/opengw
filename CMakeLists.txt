cmake_minimum_required(VERSION 3.21)

project(opengw LANGUAGES C CXX)

# OpenGW on the VectorPie SDK. Builds, from WSL2 Debian:
#   cmake --preset pc && cmake --build --preset pc     # build-pc/opengw.exe
#   cmake --preset pi && cmake --build --preset pi     # build-pi/opengw (arm64)
#   cmake --build --preset pi --target vpk             # build-pi/opengw-<version>.vpk

# The installed VectorPie SDK (`cmake --install` in the SDK: CMAKE_PREFIX_PATH
# at ~/VectorPie/dist/pc or /pi, as the presets set it). Reinstall it after
# changing the SDK, then rebuild the game.
find_package(VectorPie 0.1 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB OPENGW_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(opengw ${OPENGW_SOURCES})
vp_program(opengw)
# USE_DVG_RENDERER: the board's look and detail, now on every output, the
# window showing what the board shows.
target_compile_definitions(opengw PRIVATE USE_DVG_RENDERER)
# The original's warning set (its makefile: -Wall -Wextra), less what the
# old code trips everywhere.
target_compile_options(opengw PRIVATE -Wno-unused-parameter -Wno-unused-variable
                       -Wno-unused-but-set-variable -Wno-unused-function
                       -Wno-implicit-fallthrough -Wno-switch -Wno-sign-compare
                       -Wno-missing-field-initializers -Wno-format-truncation)

# The game's files next to the executable, where vp_asset_path finds them.
set(OPENGW_FILES game.ini leds.cfg)
add_custom_command(TARGET opengw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sounds
            $<TARGET_FILE_DIR:opengw>/sounds
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENGW_FILES} $<TARGET_FILE_DIR:opengw>/
    COMMAND ${CMAKE_COMMAND} -E copy images/opengw.png $<TARGET_FILE_DIR:opengw>/marquee.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# The game package, for Settings -> GAMES -> INSTALL GAMES. The Pi build only.
set(VPK_STAGE ${CMAKE_BINARY_DIR}/vpk/opengw)
add_custom_target(vpk
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/vpk
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VPK_STAGE}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:opengw> ${VPK_STAGE}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory sounds ${VPK_STAGE}/sounds
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENGW_FILES} ${VPK_STAGE}/
    COMMAND ${CMAKE_COMMAND} -E copy images/opengw.png ${VPK_STAGE}/marquee.png
    COMMAND sh ${VECTORPIE_PACK} ${VPK_STAGE} ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS opengw
    VERBATIM)
