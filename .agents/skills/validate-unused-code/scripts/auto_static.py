#!/usr/bin/env python3
import os
import re
import subprocess
import sys

SRC_DIR = "src"
LOG_PATH = "/tmp/yq2_missing_prototypes.log"

VALID_TYPES = {
    'void', 'int', 'float', 'double', 'char', 'short', 'long', 'unsigned',
    'qboolean', 'vec_t', 'vec3_t', 'edict_t', 'gitem_t', 'mmove_t', 'cplane_t',
    'struct', 'ipfilter_t', 'pmove_t', 'usercmd_t', 'client_t'
}

def run_build():
    print("Running mandatory post-execution build verification...")
    cmd = 'make clean >/dev/null 2>&1 && DEBUG=1 CC="ccache gcc -Wmissing-prototypes " make -j$(nproc 2>/dev/null || echo 4)'
    result = subprocess.run(cmd, shell=True)
    return result.returncode == 0

def get_all_source_files():
    files = []
    for root, dirs, filenames in os.walk(SRC_DIR):
        for f in filenames:
            if f.endswith(('.c', '.h')):
                files.append(os.path.join(root, f))
    return files

def count_word_in_file(filepath, word):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    return len(re.findall(r'\b' + re.escape(word) + r'\b', content))

def is_file_local(func_name, target_file, all_files):
    total_count = 0
    file_count = 0
    for filepath in all_files:
        count = count_word_in_file(filepath, func_name)
        total_count += count
        if filepath == target_file:
            file_count = count
    return total_count == file_count and file_count >= 1

def is_valid_return_type_line(line):
    stripped = line.strip()
    if not stripped or stripped.startswith(('if', 'while', 'for', 'return', 'switch', 'case', 'static', '#', '//', '/*')):
        return False
    # Check if first word or token is a known type
    first_token = re.split(r'[\s\*]+', stripped)[0]
    return first_token in VALID_TYPES or any(t in stripped for t in VALID_TYPES)

def add_static_to_function(filepath, func_name):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    modified = False
    i = 0
    while i < len(lines):
        line = lines[i]
        if re.search(r'\b' + re.escape(func_name) + r'\s*\(', line):
            ahead_text = "".join(lines[i:min(i+10, len(lines))])
            semicolon_idx = ahead_text.find(';')
            brace_idx = ahead_text.find('{')
            
            is_definition = False
            if brace_idx != -1 and (semicolon_idx == -1 or brace_idx < semicolon_idx):
                is_definition = True
            elif semicolon_idx == -1:
                is_definition = True

            if is_definition:
                preceding = "".join(lines[max(0, i-3):i+1])
                if 'static' not in preceding:
                    target_idx = i
                    if i > 0 and is_valid_return_type_line(lines[i-1]):
                        target_idx = i - 1
                    elif not is_valid_return_type_line(lines[i]):
                        # If neither current nor previous line is a valid return type line, skip
                        target_idx = -1

                    if target_idx != -1 and is_valid_return_type_line(lines[target_idx]):
                        if 'static' not in lines[target_idx]:
                            lines[target_idx] = 'static ' + lines[target_idx]
                            modified = True
                            print(f"Marked static definition: {func_name} in {filepath} (line {target_idx+1})")
                            break
        i += 1

    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(lines)
    return modified

def main():
    print("Generating prototype warnings log...")
    subprocess.run('make clean >/dev/null 2>&1 || true', shell=True)
    build_cmd = 'DEBUG=1 CC="ccache gcc -Wmissing-prototypes " make -j$(nproc 2>/dev/null || echo 4) > ' + LOG_PATH + ' 2>&1 || true'
    subprocess.run(build_cmd, shell=True)

    if not os.path.exists(LOG_PATH):
        print(f"Build log {LOG_PATH} not found.")
        sys.exit(1)

    all_files = get_all_source_files()

    warnings = []
    pattern = re.compile(r'^(src/[^:]+):\d+:\d+: warning: no previous prototype for ‘([^’]+)’.*[-Wmissing-prototypes]', re.MULTILINE)
    
    with open(LOG_PATH, 'r', encoding='utf-8', errors='ignore') as f:
        log_content = f.read()

    for match in pattern.finditer(log_content):
        filepath = match.group(1)
        func_name = match.group(2)
        warnings.append((filepath, func_name))

    print(f"Found {len(warnings)} missing prototype warnings.")

    local_count = 0
    refactored_count = 0

    for filepath, func_name in warnings:
        if not os.path.exists(filepath):
            continue
        if is_file_local(func_name, filepath, all_files):
            local_count += 1
            if add_static_to_function(filepath, func_name):
                refactored_count += 1

    print(f"Identified {local_count} file-local functions. Successfully refactored {refactored_count}.")

    # Mandatory Post-Execution Build Verification
    success = run_build()
    if success:
        print("Build verification PASSED successfully.")
    else:
        print("Build verification FAILED!")
        sys.exit(1)

if __name__ == '__main__':
    main()
