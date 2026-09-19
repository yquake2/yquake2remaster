#!/usr/bin/env python3
import os
import sys

def safe_float(val, default=0.0):
    try:
        return float(val)
    except (ValueError, TypeError):
        return default

def safe_int(val, default=0):
    try:
        return int(float(val))
    except (ValueError, TypeError):
        return default

def parse_entity_line(line):
    parts = line.strip().split('|')
    if len(parts) != 36:
        return None

    return {
        "classname": parts[0],
        "model_path": parts[1],
        "scale": [safe_float(parts[2]), safe_float(parts[3]), safe_float(parts[4])],
        "entity_type": parts[5],
        "mins": [safe_float(parts[6]), safe_float(parts[7]), safe_float(parts[8])],
        "maxs": [safe_float(parts[9]), safe_float(parts[10]), safe_float(parts[11])],
        "noshadow": parts[12],
        "solidflag": safe_int(parts[13]),
        "walk_speed": safe_float(parts[14]),
        "run_speed": safe_float(parts[15]),
        "speed": safe_int(parts[16]),
        "lighting": safe_int(parts[17]),
        "blending": safe_int(parts[18]),
        "target_sequence": parts[19],
        "misc_value": safe_int(parts[20]),
        "no_mip": safe_int(parts[21]),
        "spawn_sequence": parts[22],
        "description": parts[23],
        "color": [
            safe_float(parts[24]),
            safe_float(parts[25]),
            safe_float(parts[26])
        ],
        "health": safe_int(parts[27]),
        "mass": safe_int(parts[28]),
        "damage": safe_int(parts[29]),
        "damage_range": safe_int(parts[30]),
        "damage_aim": [
            safe_float(parts[31]),
            safe_float(parts[32]),
            safe_float(parts[33])
        ],
        "gib_type": parts[34],
        "gib_health": safe_int(parts[35])
    }

def parse_entity_dat(filepath):
    entities = []
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.", file=sys.stderr)
        sys.exit(1)

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            raw_line = line.strip()
            if not raw_line or raw_line.startswith('//') or raw_line.startswith(';'):
                continue
            ent = parse_entity_line(line)
            if ent:
                entities.append(ent)

    return entities

def entity_to_parts(ent):
    classname = str(ent.get("classname", ""))
    model_path = str(ent.get("model_path", ""))

    scale = ent.get("scale", [1.0, 1.0, 1.0])
    scale_x = scale[0] if len(scale) > 0 else 1.0
    scale_y = scale[1] if len(scale) > 1 else 1.0
    scale_z = scale[2] if len(scale) > 2 else 1.0

    entity_type = str(ent.get("entity_type", "general"))

    mins = ent.get("mins", [0.0, 0.0, 0.0])
    mins_x = mins[0] if len(mins) > 0 else 0.0
    mins_y = mins[1] if len(mins) > 1 else 0.0
    mins_z = mins[2] if len(mins) > 2 else 0.0

    maxs = ent.get("maxs", [0.0, 0.0, 0.0])
    maxs_x = maxs[0] if len(maxs) > 0 else 0.0
    maxs_y = maxs[1] if len(maxs) > 1 else 0.0
    maxs_z = maxs[2] if len(maxs) > 2 else 0.0

    noshadow = str(ent.get("noshadow", "shadow"))
    solidflag = str(int(ent.get("solidflag", 0)))
    walk_speed = str(ent.get("walk_speed", 0.0))
    run_speed = str(ent.get("run_speed", 0.0))
    speed = str(int(ent.get("speed", 0)))
    lighting = str(int(ent.get("lighting", 0)))
    blending = str(int(ent.get("blending", 0)))
    target_sequence = str(ent.get("target_sequence", "0:0"))
    misc_value = str(int(ent.get("misc_value", 0)))
    no_mip = str(int(ent.get("no_mip", 0)))

    spawn_sequence = str(ent.get("spawn_sequence", "none"))
    description = str(ent.get("description", ""))

    color = ent.get("color", [0.0, 0.0, 0.0])
    color_r = color[0] if len(color) > 0 else 0.0
    color_g = color[1] if len(color) > 1 else 0.0
    color_b = color[2] if len(color) > 2 else 0.0

    health = str(int(ent.get("health", 0)))
    mass = str(int(ent.get("mass", 0)))
    damage = str(int(ent.get("damage", 0)))
    damage_range = str(int(ent.get("damage_range", 0)))

    damage_aim = ent.get("damage_aim", [0.0, 0.0, 0.0])
    damage_aim_x = damage_aim[0] if len(damage_aim) > 0 else 0.0
    damage_aim_y = damage_aim[1] if len(damage_aim) > 1 else 0.0
    damage_aim_z = damage_aim[2] if len(damage_aim) > 2 else 0.0

    gib_type = str(ent.get("gib_type", "none"))
    gib_health = str(int(ent.get("gib_health", 0)))

    return [
        classname,
        model_path,
        str(scale_x), str(scale_y), str(scale_z),
        entity_type,
        str(mins_x), str(mins_y), str(mins_z),
        str(maxs_x), str(maxs_y), str(maxs_z),
        noshadow,
        solidflag,
        str(walk_speed), str(run_speed),
        speed,
        lighting,
        blending,
        target_sequence,
        misc_value,
        no_mip,
        spawn_sequence,
        description,
        str(color_r), str(color_g), str(color_b),
        health,
        mass,
        damage,
        damage_range,
        str(damage_aim_x), str(damage_aim_y), str(damage_aim_z),
        gib_type,
        gib_health
    ]

def entities_equal(e1, e2):
    fields = [
        "classname", "model_path", "scale", "entity_type", "mins", "maxs",
        "noshadow", "solidflag", "walk_speed", "run_speed", "speed", "lighting",
        "blending", "target_sequence", "misc_value", "no_mip", "spawn_sequence",
        "description", "color", "health", "mass", "damage", "damage_range",
        "damage_aim", "gib_type", "gib_health"
    ]
    for field in fields:
        v1 = e1.get(field)
        v2 = e2.get(field)
        if isinstance(v1, list) and isinstance(v2, list):
            if len(v1) != len(v2):
                return False
            for a, b in zip(v1, v2):
                if float(a) != float(b):
                    return False
        else:
            try:
                if float(v1) != float(v2):
                    return False
            except (ValueError, TypeError):
                if str(v1) != str(v2):
                    return False
    return True
