cmake_minimum_required(VERSION 3.4)

project(@model-name@)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

set(LIB_SRC @model-name@.c ni_modelframework.c)
add_library(@model-name@ SHARED ${LIB_SRC})

