cmake_minimum_required(VERSION 3.16)

# Add Bluepad32 and BTstack component directories
set(EXTRA_COMPONENT_DIRS 
    "components/bluepad32/src/components"
    "components/bluepad32/external/btstack/port/esp32/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(king-tiger-idf)
