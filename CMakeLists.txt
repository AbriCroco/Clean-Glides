cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(CleanGlides VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        "$<$<CONFIG:Debug>:-fsanitize=address>"
        "$<$<CONFIG:Debug>:-fsanitize=undefined>"
    )

    add_link_options(
        "$<$<CONFIG:Debug>:-fsanitize=address>"
        "$<$<CONFIG:Debug>:-fsanitize=undefined>"
    )
endif()

# generate PDB/dSYM files for the Release build
if(MSVC)
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")
endif()

# We're going to use CPM as our package manager to bring in JUCE
# Check to see if we have CPM installed already.  Bring it in if we don't.
set(CPM_DOWNLOAD_VERSION 0.43.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif ()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 9.0.5
)

set(CG_COMPANY "AbriCroco")
set(CG_MANUFACTURER_CODE AbCr)
set(CG_PLUGIN_CODE Cgld)

# --- Target 1: VST3 (synth-flagged) ---------------------------
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME ${CG_COMPANY}
    PLUGIN_MANUFACTURER_CODE ${CG_MANUFACTURER_CODE}
    PLUGIN_CODE ${CG_PLUGIN_CODE}
    BUNDLE_ID "com.abricroco.cleanglides"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3
    PRODUCT_NAME "Clean Glides"
)

# --- Target 2: AU MIDI FX (for Logic's MIDI FX) -----------------------
juce_add_plugin(${PROJECT_NAME}_MIDIFX
    COMPANY_NAME ${CG_COMPANY}
    PLUGIN_MANUFACTURER_CODE ${CG_MANUFACTURER_CODE}
    PLUGIN_CODE ${CG_PLUGIN_CODE}
    BUNDLE_ID "com.abricroco.cleanglides.midifx"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS AU
    PRODUCT_NAME "Clean Glides"
)

juce_add_binary_data(BinaryData SOURCES
    assets/fonts/Monoton-Regular.ttf
    assets/fonts/Orbitron-Regular.ttf
    assets/fonts/Orbitron-Medium.ttf
    assets/fonts/Orbitron-SemiBold.ttf
    assets/fonts/Orbitron-Bold.ttf
)

foreach(target ${PROJECT_NAME} ${PROJECT_NAME}_MIDIFX)
    target_sources(${target} PRIVATE
        source/PluginEditor.h
        source/PluginEditor.cpp
        source/PluginProcessor.h
        source/PluginProcessor.cpp
        source/MPEPortamento.h
        source/MPEPortamento.cpp
        source/CurveComponent.h
        source/CurveComponent.cpp
        source/CurveUtils.h
        source/KnobComponent.h
        source/KnobComponent.cpp
        source/LookAndFeel.h
        source/LookAndFeel.cpp
    )

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=0
        DONT_SET_USING_JUCE_NAMESPACE=1
    )

    target_link_libraries(${target}
        PRIVATE
            BinaryData
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce_recommended_warning_flags
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
    )
endforeach()
