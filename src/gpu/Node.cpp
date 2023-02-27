#include "minipeak/gpu/Node.h"
#include "assert.h"
#include "algorithm"
#include "numeric"
#include "sstream"

namespace GPU {
    DispatchRange Node::work_size() const {
        if(set_ws[0] != 0) return set_ws;

        auto factory = settingsFactory();
        auto platform_setting = factory(platform_name() + "_" + key());
        auto key_setting = factory(key());
        auto x = platform_setting("work_size_x", key_setting("work_size_x", 0));
        auto y = platform_setting("work_size_y", key_setting("work_size_y", 1));
        auto z = platform_setting("work_size_z", key_setting("work_size_z", 1));
        if(x != 0) {
            set_ws = GPU::DispatchRange(x,y,z);
            return set_ws;
        }
#ifdef cl_khr_suggested_local_work_size

#endif

        auto run_size = global_size();
        auto max_local_size = std::min(512, max_workgroup_size());
        auto next_local_size = max_local_size;
        int range[3] = { 0 };
        for(int i = 0;i < 3;i++) {
            range[i] = std::gcd(run_size[i], next_local_size);
            next_local_size = next_local_size / range[i];
        }

        set_ws = GPU::DispatchRange(range[0], range[1], range[2]);
        printf("Node %s has global size of %d, %d, %d and local size of %d, %d, %d (max of %d)\n", name().c_str(),
               run_size[0], run_size[1], run_size[2], range[0], range[1], range[2], max_local_size);
        return set_ws;
    }

    void Node::set_work_size(const DispatchRange &ws) {
        if(ws[0] != 0) {
            assert(is_ws_valid(ws));
            set_ws = ws;
        }
    }

    void Node::operator()() {
        if(!is_built()) {
            build();
        }
    }

  DispatchRange Node::max_workgroup_count() const {
    return DispatchRange(65535, 65535, 65535);    
  }
  
    bool Node::is_ws_valid(const DispatchRange &ws) const {
        auto gs = global_size();
	auto max_gs = max_workgroup_count();	
        for(int i = 0;i < 3;i++) {
	  if((gs[i] % ws[i]) != 0 || (gs[i] / ws[i]) > max_gs[i])
                return false;
        }
        return true;
    }

    std::string Node::key() const {
        auto gws = global_size();
        return std::to_string(gws[0]) + "x" + std::to_string(gws[1]) + "x" + std::to_string(gws[2]);
    }

    const PlatformSettingsFactory &Node::settingsFactory() const {
        static PlatformSettingsFactory factory({});
        return factory;
    }

    DispatchRange::DispatchRange(int x, int y, int z) : std::array<int, 3>({x, y, z}){
    }


    std::string TileableNode::preamble() const {
        std::stringstream ss;
        ss << Node::preamble() << std::endl;
        ss << "#define IS_TILED " << is_tiled() << std::endl;
        auto gts = this->global_tile_size();
        if(is_tiled()) {
            ss << "#define get_tile_x() get_global_id(0) " << std::endl;
            ss << "#define get_tile_y() get_global_id(1) " << std::endl;
            ss << "#define get_tile_z() get_global_id(2) " << std::endl;
            ss << "#define get_tile_idx() (get_global_id(0) + get_global_id(1) * " << (gts[0]) << "u + get_global_id(2) * " << (gts[0] * gts[1]) << "u) " << std::endl;
        } else {
            ss << "#define get_tile_x() (get_global_id(0) % " << gts[0] << "u) " << std::endl;
            ss << "#define get_tile_y() ((get_global_id(0) / " << gts[0] << "u) % " << gts[1] << "u)" << std::endl;
            ss << "#define get_tile_z() (get_global_id(0) / (" << gts[0] * gts[1] << "u))" << std::endl;
            ss << "#define get_tile_idx() get_global_id(0)" << std::endl;
        }
        return ss.str();
    }

    // TileableNode::TileableNode() {
    //   printf("Ctor1\n");
    // }

    TileableNode::TileableNode(bool is_tiled) : _is_tiled(is_tiled) {

    }

    GPU::DispatchRange TileableNode::global_size() const {
        if(is_tiled()) {
            return global_tile_size();
        } else {
            auto gts = global_tile_size();
            return {gts[0] * gts[1] * gts[2], 1 ,1};
        }
    }

    std::string TileableNode::key() const {
        auto gws = global_tile_size();
        return std::to_string(gws[0]) + "x" + std::to_string(gws[1]) + "x" + std::to_string(gws[2]);
    }

    std::string TileableNode::name() const {
        auto gws = work_size();
        return "Tiled: " + std::to_string(is_tiled()) + " " + std::to_string(gws[0]) + "x" + std::to_string(gws[1]) + "x" + std::to_string(gws[2]);
    }

    bool TileableNode::is_tiled() const {
        if(!_is_tiled.has_value()) {
            auto factory = settingsFactory();
            auto platform_setting = factory(platform_name() + "_" + key());
            auto key_setting = factory(key());

            _is_tiled = (bool)platform_setting("tiled", key_setting("tiled", 1));
        }
        return _is_tiled.value();
    }

    TileableNode::TileableNode(std::optional<bool> is_tiled) : _is_tiled(is_tiled) {

    }

}
