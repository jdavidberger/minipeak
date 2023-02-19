#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include "stdio.h"

#include "minipeak/opengl/Context.h"

static void DEBUGPROC(GLenum source,
               GLenum type,
               GLuint id,
               GLenum severity,
               GLsizei length,
               const GLchar *message,
               const void *userParam) {
    const char* severity_cstr = "Unknown";
    switch(severity) {
        case GL_DEBUG_SEVERITY_HIGH: severity_cstr = "High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severity_cstr = "Medium"; break;
        case GL_DEBUG_SEVERITY_LOW: severity_cstr = "Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_cstr = "Notification"; break;
    }

    if(severity != GL_DEBUG_SEVERITY_HIGH) {
        printf("gl error %24s %d %s\n", severity_cstr, id, message);
    } else {
        fprintf(stderr, "gl error %d %s\n", id, message);
    }
}

static void error_cb(int error_code, const char* description) {
    printf("glfw error %d %s\n", error_code, description);
}

void OGL::EnsureGLContext() {
    glfwSetErrorCallback(error_cb);
    if (!glfwInit()) {
        const char *error_msg = 0;
        glfwGetError(&error_msg);
        printf("Could not init glfw %s", error_msg);
        return;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *offscreen_context = glfwCreateWindow(640, 480, "", NULL, NULL);

    glfwMakeContextCurrent(offscreen_context);

    GLint major = 0, minor = 0;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    printf("GL version: %d.%d\n", major, minor);
    printf("GL Shading language version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor %s\n", glGetString(GL_VENDOR));
    printf("Renderer '%s' %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);

    for (GLint i = 0; i < n; i++) {
        const char *extension =
                (const char *) glGetStringi(GL_EXTENSIONS, i);
        //printf("Ext %d: %s\n", i, extension);
    }

    int flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DEBUGPROC, 0);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
    }
}
