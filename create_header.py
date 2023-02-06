import sys
from pathlib import Path

fn = sys.argv[1]
output_fn = f"{fn}.h"

if len(sys.argv) > 2:
    output_fn = sys.argv[2]
    target_path = Path(output_fn).parent
    print(target_path)
    target_path.mkdir(parents=True, exist_ok=True)

name = fn.split('/')[-1]

if name.endswith(".glsl"):
    file = open(fn, mode='r')

    with open(output_fn, mode='w') as f:
        f.write(f"""#pragma once
    static const char* {name.replace('.', '_')} = R"18e3c792b59(
    {file.read()}
    )18e3c792b59";
    """)
else:
    file = open(fn, mode='rb')

    with open(output_fn, mode='w') as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n")
        f.write(f"static const uint8_t {name.replace('.', '_')}[] = {{")
        buf = file.read()
        for idx, b in enumerate(buf):
            if idx % 16 == 0:
                f.write("\n")
            f.write(f"{b:#04x}, ")
        f.write("};\n\n")
