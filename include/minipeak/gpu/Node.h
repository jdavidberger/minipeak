#pragma once

#include "string"
#include "PlatformSettings.h"

namespace GPU {
struct DispatchRange : public std::array<int, 3>{
    DispatchRange(int x = 1, int y = 1, int z = 1);
    };

    struct Node {
    protected:
        mutable DispatchRange set_ws = {0};
    public:
        virtual DispatchRange global_size() const = 0;
        virtual DispatchRange work_size() const;
        virtual void operator()() = 0;

        virtual void set_work_size(const DispatchRange& ws);
        bool is_ws_valid(const DispatchRange& ws) const;

        virtual bool is_built() const = 0;
        virtual void build() = 0;
        virtual void reset_build() = 0;      

        virtual void sync() = 0;
        virtual int max_workgroup_size() const = 0;
        virtual int max_effective_workgroup_size() const { return max_workgroup_size(); }
        virtual int preferred_work_group_size_multiple() const = 0;
        virtual DispatchRange max_workgroup_count() const;
      
        virtual std::string key() const;
        virtual const PlatformSettingsFactory& settingsFactory() const;

        virtual std::string preamble() const { return ""; }

        virtual std::string platform_name() const = 0;
        virtual std::string name() const = 0;
    };


    class TileableNode : virtual public Node {
        mutable std::optional<bool> _is_tiled;
    public:
        bool is_tiled() const;
        void set_is_tiled(bool v) { _is_tiled = v; }

        TileableNode();
        explicit TileableNode(bool is_tiled);
        explicit TileableNode(std::optional<bool> is_tiled);
        std::string preamble() const override;

        GPU::DispatchRange global_size() const override;
        virtual GPU::DispatchRange global_tile_size() const = 0;

        std::string key() const override;
      std::string name() const override;      
    };
}
