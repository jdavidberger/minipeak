function(create_app name mainlib)
    if(TARGET ${mainlib})
        add_executable(${name} ${name}.cpp)
        target_link_libraries(${name} ${mainlib} ${ARGN})
        install(TARGETS ${name} RUNTIME_DEPENDENCY_SET minipeak-helpers DESTINATION bin)
    else()
        message("${name} not building since ${mainlib} isn't built")
    endif()
endfunction()

create_app(glminipeak minipeak-helpers-gl z)
create_app(glmempeak minipeak-helpers-gl z)
create_app(clminipeak minipeak-helpers-cl z)
create_app(clmempeak minipeak-helpers-cl z)

generate_spv_headers(spv_headers
        shaders/compute_sp_v1.glsl
        shaders/compute_sp_v2.glsl
        shaders/compute_sp_v4.glsl
        shaders/compute_sp_v8.glsl
        shaders/compute_sp_v16.glsl
        )
message("Headers: ${spv_headers}")
create_app(vkminipeak minipeak-helpers-vk z)
set_property(SOURCE vkminipeak.cpp APPEND PROPERTY OBJECT_DEPENDS ${spv_headers})

execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE KERNEL_RELEASE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_path(KERNELHEADERS_DIR
  linux/user.h
  PATHS /usr/src/linux-headers-${KERNEL_RELEASE}/include /lib/modules/${KERNEL_RELEASE}/build/include
)
find_file(DRM_HEADER xf86drmMode.h PATHS ${KERNELHEADERS_DIR})

if(DRM_HEADER)
    if(KERNELHEADERS_DIR)
      add_executable(panfrost_drmDump panfrost_drmDump.c)
      target_compile_definitions(panfrost_drmDump PUBLIC -D__user=)
      target_link_libraries(panfrost_drmDump drm)
      target_include_directories(panfrost_drmDump PUBLIC ${KERNELHEADERS_DIR}/uapi/drm)
    endif()
endif()