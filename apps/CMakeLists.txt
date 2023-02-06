function(create_app name mainlib)
    if(TARGET ${mainlib})
        add_executable(${name} ${name}.cpp)
        target_link_libraries(${name} ${mainlib} ${ARGN})
        install(TARGETS ${name} RUNTIME_DEPENDENCY_SET depthlib DESTINATION bin)
    else()
        message("${name} not building since ${mainlib} isn't built")
    endif()
endfunction()

create_app(glminipeak minipeak-helpers-gl z)
create_app(clminipeak minipeak-helpers-cl z)

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
