# Builds the Komawari reader (the C program in reader/).
#
# There are two builds, chosen with a preset from CMakePresets.json:
#   cmake --preset desktop && cmake --build --preset desktop    a window on this computer
#   cmake --preset vita    && cmake --build --preset vita       a package for the PS Vita
# Any build without a preset is a desktop build.
cmake_minimum_required(VERSION 3.26)

# The vita preset sets KOMAWARI_VITA. The compiler for the Vita is described by a
# "toolchain file" inside the Vita SDK, and it must be chosen before project().
# The SDK is found through the VITASDK variable, or else in the home folder.
if(KOMAWARI_VITA AND NOT CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(vita_sdk "$ENV{VITASDK}")
  else()
    set(vita_sdk "$ENV{HOME}/vitasdk")
  endif()
  set(CMAKE_TOOLCHAIN_FILE "${vita_sdk}/share/vita.toolchain.cmake" CACHE PATH "Compiler setup")
endif()

project(komawari_reader C)

set(CMAKE_C_STANDARD 11)
# Writes compile_commands.json, which lets the IDE understand the code.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The Vita toolchain file sets VITASDK, so this tells the two builds apart.
if(DEFINED VITASDK)
  set(BUILDING_FOR_VITA ON)
  # libwebp, which SDL2_image brings along, needs the thread library, and on the
  # Vita a library must be named after the ones that use it. This puts it last.
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread")
else()
  set(BUILDING_FOR_VITA OFF)
endif()

# On Apple Silicon Macs, Homebrew installs SDL2 here.
if(APPLE AND EXISTS "/opt/homebrew")
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

# cJSON reads JSON. It is a git submodule; only this one file of it is built.
add_library(cjson STATIC third_party/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC third_party/cJSON)

add_executable(reader
  reader/main.c      # start-up; moves between the shelf and the reading screen
  reader/library.c   # finds the comics, one folder each
  reader/shelf.c     # the screen for choosing a comic
  reader/reading.c   # the screen for reading one, panel by panel
  reader/book.c      # reads a comic's panels.json
  reader/file.c      # reads a whole file from the memory card
  reader/pages.c     # keeps the pictures of the pages ready, loading them in the background
  reader/picture.c   # loads a picture file and logs how long that took
  reader/shot.c      # how a page is framed on the screen
  reader/motion.c    # timing of movements
  reader/draw.c      # simple shapes
  reader/input.c     # keys and buttons
)
target_compile_options(reader PRIVATE -Wall -Wextra)
target_link_libraries(reader PRIVATE cjson SDL2::SDL2)
# The Vita SDK only has the static form of SDL2_image.
if(TARGET SDL2_image::SDL2_image)
  target_link_libraries(reader PRIVATE SDL2_image::SDL2_image)
else()
  target_link_libraries(reader PRIVATE SDL2_image::SDL2_image-static)
endif()

# Where the reader looks for comics, one folder each, when it is started
# without an argument: on the Vita a folder on the memory card, on the desktop
# the folder that `komawari prepare` fills.
set(prepared_comics "${CMAKE_SOURCE_DIR}/for_vita")
if(BUILDING_FOR_VITA)
  set(default_library "ux0:data/komawari")
else()
  set(default_library "${prepared_comics}")
endif()
target_compile_definitions(reader PRIVATE DEFAULT_LIBRARY_DIR="${default_library}")

if(BUILDING_FOR_VITA)
  include("${VITASDK}/share/vita.cmake" REQUIRED)

  # What the Vita shows for the reader on its home screen: the round bubble
  # (icon0), the loading screen (pic0) and the page behind the bubble with its
  # start button. The pictures are drawn by `uv run komawari artwork`.
  set(SCE_SYS "${CMAKE_SOURCE_DIR}/reader/sce_sys")

  vita_create_self(reader.self reader)
  # The title id must be four capital letters and five digits.
  vita_create_vpk(komawari.vpk "KMWR00001" reader.self
    VERSION "01.00"
    NAME "Komawari"
    FILE ${SCE_SYS}/icon0.png sce_sys/icon0.png
    FILE ${SCE_SYS}/pic0.png sce_sys/pic0.png
    FILE ${SCE_SYS}/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE ${SCE_SYS}/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE ${SCE_SYS}/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  )

  # The "emulator" target builds the package, then installs and starts it in Vita3K.
  # It opens Vita3K's full interface on purpose: its plain window (--frontend sdl)
  # does not turn keyboard keys into Vita buttons.
  find_program(VITA3K NAMES Vita3K vita3k
    PATHS "/Applications/Vita3K.app/Contents/MacOS" "$ENV{HOME}/Applications/Vita3K.app/Contents/MacOS")
  if(VITA3K)
    add_custom_target(emulator
      COMMAND "${VITA3K}" "${CMAKE_CURRENT_BINARY_DIR}/komawari.vpk"
      USES_TERMINAL
      COMMENT "Starting the reader in Vita3K (quit Vita3K to finish)")
    add_dependencies(emulator komawari.vpk-vpk)

    # Before that, bring the comics on the emulator's memory card up to date
    # with the prepared ones. (This is where Vita3K keeps its memory card on
    # macOS; on other systems the comics have to be copied by hand.)
    set(emulator_card "$ENV{HOME}/Library/Application Support/Vita3K/Vita3K/fs/ux0")
    if(IS_DIRECTORY "${emulator_card}")
      add_custom_target(emulator_comics
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different
                "${prepared_comics}" "${emulator_card}/data/komawari"
        COMMENT "Copying the prepared comics to the emulator's memory card")
      add_dependencies(emulator emulator_comics)
    endif()
  else()
    message(STATUS "Vita3K not found: no emulator target")
  endif()
endif()
