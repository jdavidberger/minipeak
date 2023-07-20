import sys
from pathlib import Path

fn = sys.argv[1]
output_fn = f"{fn}.cpp"

if len(sys.argv) > 2:
    output_fn = sys.argv[2]
    target_path = Path(output_fn).parent
    print(target_path)
    target_path.mkdir(parents=True, exist_ok=True)

name = fn.split('/')[-1]

file = open(fn, mode='r')

with open(output_fn, mode='w') as f:
    f.write(f"""#pragma once
#include "minipeak/opengl/shaderdb.h"

static const char* {name.replace('.', '_')} = R"18e3c792b59(
{file.read()}
)18e3c792b59";

void __attribute__((constructor)) register_{name.replace('.', '_')}() {{
    ShaderDB::g().Register("{name}", {name.replace('.', '_')});
}}
""")
