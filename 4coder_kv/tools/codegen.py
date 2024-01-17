#! /usr/local/bin/python3

scene_header = [('u32', "magic_number"),
                ('u64', "version"),]

def struct(name, fields):
    print(f"struct {name} {{")
    for field in scene_header + fields:
        print('  ', end='')
        print(f"{field[0]} {field[1]};")
    print("} __attribute__((packed));")

def generate_scene_structs():
    old_scene_fields = [('v3', 'eye_position'),
                        ('b32', 'eye_inited'),]
    new_scene_fields = [('v3', 'eye_position'),
                        ('b32', 'eye_inited'),]

    print("!!! REMEMBER TO BUMP VERSION NUMBER !!!\n")

    struct("OldScene", old_scene_fields)
    print()
    struct("Scene", old_scene_fields)
    print()
    # auto-conversion code
    for field in new_scene_fields:
        print(f"state.scene.{field[1]} = old_scene.{field[1]};")

for char in range(ord('A'), ord('Z')+1):
    print(f"case '{chr(char)}':")
