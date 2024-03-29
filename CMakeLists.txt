cmake_minimum_required(VERSION 3.20)
project(minipeak)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_CXX_STANDARD 17)

add_definitions(-DGL_GLEXT_PROTOTYPES=1)
find_package(OpenGL 4.3 REQUIRED COMPONENTS OpenGL EGL)
find_package(GLEW REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})

find_program(PYTHON python3 python REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated ${CMAKE_CURRENT_BINARY_DIR}/generated/filters/shaders)

macro(generate_header input)
  get_filename_component(infile ${input} ABSOLUTE)

  SET(infile_build ${CMAKE_CURRENT_BINARY_DIR}/generated/${input})
  SET(outfile_hpp ${infile_build}.h)

  add_custom_command(OUTPUT ${outfile_hpp}
          COMMAND ${PYTHON}
          ARGS ${CMAKE_CURRENT_SOURCE_DIR}/create_header.py ${infile} ${CMAKE_CURRENT_BINARY_DIR}/generated/${input}.h
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          DEPENDS ${infile} ${CMAKE_CURRENT_SOURCE_DIR}/create_header.py)
endmacro()

set(CREATE_SHADER_REGISTER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/create_shader_register.py CACHE PATH "" FORCE)
function(generate_shader_header input)
  get_filename_component(infile ${input} ABSOLUTE)

  SET(infile_build ${CMAKE_CURRENT_BINARY_DIR}/generated/${input})
  SET(outfile_hpp ${infile_build}.cpp)

  add_custom_command(OUTPUT ${outfile_hpp}
          COMMAND ${PYTHON}
          ARGS ${CREATE_SHADER_REGISTER_FILE} ${infile} ${CMAKE_CURRENT_BINARY_DIR}/generated/${input}.cpp
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          DEPENDS ${infile} ${CREATE_SHADER_REGISTER_FILE})
endfunction()

macro(generate_headers OUT_NAME)
  set(RTN "")
  foreach(fn ${ARGN})
    generate_header(${fn})
    list(APPEND RTN ${CMAKE_CURRENT_BINARY_DIR}/generated/${fn}.h)
  endforeach()
  SET(${OUT_NAME} ${RTN})
endmacro()

macro(generate_shader_lib OUT_NAME)
  set(RTN "")
  foreach(fn ${ARGN})
    generate_shader_header(${fn})
    list(APPEND RTN ${CMAKE_CURRENT_BINARY_DIR}/generated/${fn}.cpp)
  endforeach()

  add_library(${OUT_NAME} SHARED ${RTN})
  target_link_libraries(${OUT_NAME} minipeak-helpers-gl)
endmacro()

find_program(GLSLC glslc)

set(CREATE_HEADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/create_header.py CACHE PATH "" FORCE)
macro(generate_spv_header input)
  get_filename_component(infile ${input} ABSOLUTE)
  get_filename_component(infile_dir ${fn} DIRECTORY)
  get_filename_component(infile_name ${input} NAME_WE)

  SET(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${infile_dir}/spv/${infile_name}.spv")
  SET(HDR_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/${infile_dir}/${infile_name}.spv.h")
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

  if(GLSLC)
      execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/${infile_dir}/.ignore RESULT_VARIABLE writeable)
      if(writeable EQUAL "0")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${infile_dir}/spv")

        add_custom_command(
                OUTPUT "${SPV_FILE}"
                COMMAND ${GLSLC} -fshader-stage=compute ${infile} --target-env=vulkan1.1 -std=310es -I ${CMAKE_CURRENT_SOURCE_DIR}/${infile_dir} -O -o "${SPV_FILE}"
                DEPENDS ${infile} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Compiling ${infile_name}")
      endif()
  endif()

  add_custom_command(OUTPUT "${HDR_FILE}"
          COMMAND ${PYTHON}
          ARGS ${CREATE_HEADER_FILE} "${SPV_FILE}" "${HDR_FILE}"
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          DEPENDS "${SPV_FILE}"

          COMMENT "Headerifying ${infile_name}")
endmacro()

macro(generate_spv_headers OUT_NAME)
  set(RTN "")
  foreach(fn ${ARGN})
    generate_spv_header(${fn})
    get_filename_component(infile_dir ${fn} DIRECTORY)
    get_filename_component(infile_name ${fn} NAME_WE)
    list(APPEND RTN ${CMAKE_CURRENT_BINARY_DIR}/generated/${infile_dir}/${infile_name}.spv.h)
  endforeach()
  SET(${OUT_NAME} ${RTN})
endmacro()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/filters/shaders)

add_library(minipeak-helpers STATIC src/gpu/BufferInfo.cpp src/gpu/Buffer.cpp src/gpu/PlatformSettings.cpp src/gpu/Node.cpp)
target_include_directories(minipeak-helpers PUBLIC include)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/minipeak  DESTINATION include)

if(TARGET GLEW::GLEW)
    generate_headers(glsl_headers
          src/opengl/data_access.glsl
          )
	
  add_library(minipeak-helpers-gl STATIC
          src/opengl/shaderdb.cpp
          src/opengl/program.cpp
          src/opengl/buffer.cpp
          src/opengl/Context.cpp
          src/opengl/Node.cpp ${glsl_headers})
  target_link_libraries(minipeak-helpers-gl GL GLEW::GLEW OpenGL::EGL glfw dl minipeak-helpers crypto ssl)
  install(TARGETS minipeak-helpers-gl RUNTIME_DEPENDENCY_SET minipeak-helpers DESTINATION lib)
endif()

FIND_PACKAGE(OpenCL)
IF(OpenCL_FOUND)
  add_library(minipeak-helpers-cl STATIC
          src/opencl/Context.cpp
          src/opencl/Buffer.cpp src/opencl/Kernel.cpp
          src/opencl/Node.cpp
          )
  target_link_libraries(minipeak-helpers-cl ${OpenCL_LIBRARY} minipeak-helpers dl )

  install(TARGETS minipeak-helpers-cl RUNTIME_DEPENDENCY_SET minipeak-helpers DESTINATION lib)
endif()

add_subdirectory(apps)
