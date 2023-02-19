#include "minipeak/opengl/Node.h"
#include <assert.h>

void OGL::Node::sync() {
    program.sync();
}

int OGL::Node::max_workgroup_size() const {
    GLint64 rtn;
    glGetInteger64v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &rtn);
    return rtn;
}


int OGL::Node::max_effective_workgroup_size() const {
  if(platform_name() == "V3D 4.2") {
    return 256;
  }
  return max_workgroup_size();
}


GPU::DispatchRange OGL::Node::max_workgroup_count() const {
    GLint rtn[3];
    for(int i = 0;i < 3;i++) 
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, rtn + i);

    return GPU::DispatchRange(rtn[0], rtn[1], rtn[2]);
}

int OGL::Node::preferred_work_group_size_multiple() const {
  if(platform_name() == "V3D 4.2") {
    return 4;
  }
  //GLint64 rtn;
    //glGetInteger64v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &rtn);
    return std::min(32, max_workgroup_size());
}

std::string OGL::Node::platform_name() const {
    auto platform = (char*)glGetString(GL_RENDERER);
    if(!platform)
        return "";
    return platform;
}

std::string OGL::Node::name() const {
  return typeid(*this).name();
}

void OGL::Node::reset_build() {
  program.ptr = 0;
}

void OGL::Node::set_work_size(const GPU::DispatchRange& ws) {
  GPU::Node::set_work_size(ws);
  reset_build();
}

void OGL::Node::operator()() {
    GPU::Node::operator()();

    auto global_size = this->global_size();
    auto work_group_size = this->work_size();
    assert(is_ws_valid(work_group_size));
    program.dispatch(global_size[0] / work_group_size[0],
                     global_size[1] / work_group_size[1],
                     global_size[2] / work_group_size[2]);
}

std::string OGL::Node::preamble() const {
    auto ws = work_size();

    std::stringstream ss;
    ss << GPU::Node::preamble() << std::endl;
    ss << "#define get_global_id(x) gl_GlobalInvocationID[x]" << std::endl;
    ss << "#define LOCAL_SIZE_X " << std::get<0>(ws) << std::endl;
    ss << "#define LOCAL_SIZE_Y " << std::get<1>(ws) << std::endl;
    ss << "#define LOCAL_SIZE_Z " << std::get<2>(ws) << std::endl;
    ss << "layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;" << std::endl;
    return ss.str();
}

std::string OGL::TileableNode::preamble() const {
    return Node::preamble() + "\n" + GPU::TileableNode::preamble() + "\n";
}

std::string OGL::TileableNode::name() const  {
    return Node::name() + " " + GPU::TileableNode::name();
}

