#!/usr/bin/env python3
import json
import sys
import os

from entity_lib import parse_entity_line, entities_equal, entity_to_parts

def main():
    json_path = "stuff/entgen/entity.json"
    output_path = "stuff/models/entity.dat"

    if len(sys.argv) > 1:
        json_path = sys.argv[1]
    if len(sys.argv) > 2:
        output_path = sys.argv[2]

    if not os.path.exists(json_path):
        print(f"Error: {json_path} not found.", file=sys.stderr)
        sys.exit(1)

    with open(json_path, 'r', encoding='utf-8') as f:
        new_entities = json.load(f)

    new_ent_map = {ent.get("classname"): ent for ent in new_entities if "classname" in ent}

    if not os.path.exists(output_path):
        print(f"Error: {output_path} not found to update.", file=sys.stderr)
        sys.exit(1)

    with open(output_path, 'r', encoding='utf-8') as f:
        original_lines = f.readlines()

    updated_lines = []
    updated_count = 0

    for line in original_lines:
        stripped = line.strip()
        if not stripped or stripped.startswith('//') or stripped.startswith(';'):
            updated_lines.append(line)
            continue

        parts = stripped.split('|')
        if len(parts) < 1:
            updated_lines.append(line)
            continue

        classname = parts[0]
        if classname in new_ent_map:
            orig_ent = parse_entity_line(line)
            new_ent = new_ent_map[classname]

            if orig_ent and entities_equal(orig_ent, new_ent):
                updated_lines.append(line)
            else:
                new_parts = entity_to_parts(new_ent)
                while len(new_parts) < len(parts):
                    new_parts.append("")
                newline_char = line[len(line.rstrip('\r\n')):]
                if not newline_char:
                    newline_char = '\n'
                updated_lines.append("|".join(new_parts) + newline_char)
                updated_count += 1
        else:
            updated_lines.append(line)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.writelines(updated_lines)

    print(f"Successfully updated {output_path} from {json_path} ({updated_count} lines updated out of {len(original_lines)} total lines)")

if __name__ == '__main__':
    main()
