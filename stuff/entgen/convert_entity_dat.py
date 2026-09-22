#!/usr/bin/env python3
import json
import sys
import os

from entity_lib import parse_entity_dat

def main():
    input_path = "stuff/models/entity.dat"
    output_path = "stuff/entgen/entity.json"
    if len(sys.argv) > 1:
        input_path = sys.argv[1]
    if len(sys.argv) > 2:
        output_path = sys.argv[2]

    entities = parse_entity_dat(input_path)
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(entities, f, indent=2)
    print(f"Successfully converted {len(entities)} entities from {input_path} to {output_path}")

if __name__ == '__main__':
    main()
