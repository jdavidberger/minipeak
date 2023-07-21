import sys
from pathlib import Path

fn = sys.argv[1]
output_fn = f"{fn}.cpp"

if len(sys.argv) > 2:
    output_fn = sys.argv[2]
    target_path = Path(output_fn).parent
    target_path.mkdir(parents=True, exist_ok=True)

name = fn.split('/')[-1]

if name.endswith(".glsl"):
    file = open(fn, mode='r')

    with open(output_fn, mode='w') as f:
        f.write(f"""
    #include "vector"
    #include "string"
    
    static const char* {name.replace('.', '_')} = R"18e3c792b59(
    {file.read()}
    )18e3c792b59";
    void ShaderDB_Register(const std::string& name, const std::string& src);
    void ShaderDB_Register(const std::string& name, const std::vector<uint8_t>& src);

    void __attribute__((constructor)) register_{name.replace('.', '_')}() {{
        ShaderDB_Register("{name}", {name.replace('.', '_')});
    }}
    """)
else:
    file = open(fn, mode='rb')
    prefix = name.split('.')[0]
    with open(output_fn, mode='w') as f:
        f.write("#include <vector>\n")
        f.write("#include <string>\n")
        f.write("#include <stdint.h>\n")

        f.write(f"""
        void ShaderDB_Register(const std::string& name, const std::string& src);
        void ShaderDB_Register(const std::string& name, const std::vector<uint8_t>& src);        
        void __attribute__((constructor)) register_{name.replace('.', '_')}() {{
        """)
        f.write(f"const std::vector<uint8_t> _{name.replace('.', '_')} = {{")
        buf = file.read()
        for idx, b in enumerate(buf):
            if idx % 16 == 0:
                f.write("\n")
            f.write(f"{b:#04x}, ")
        f.write("};\n\n")

        f.write(f"""        
             ShaderDB_Register("{prefix}", _{name.replace('.', '_')});
        }}
        """)
