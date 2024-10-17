#!/usr/bin python3

import json
import argparse

def read_json(file_path):
    with open(file_path, 'r') as file:
        return json.load(file)

def filter_json(data):
    """Recursively remove fields starting with an underscore from the JSON data."""
    if isinstance(data, dict):
        return {k: filter_json(v) for k, v in data.items() if not k.startswith('_')}
    elif isinstance(data, list):
        return [filter_json(item) for item in data]
    else:
        return data

def write_cpp_header(data, file_path):
    with open(file_path, 'w') as file:

        file.write("#pragma once\n\n")
        file.write("static constexpr uint8_t DEFAULT_OPENACE_CONFIG[] = {")

        json_string = json.dumps(data, separators=(',', ':'))

        for i, char in enumerate(json_string):
            if i % 20 == 0:
                file.write("\n    ")
            file.write(f"0x{ord(char):02X}, ")

        file.write("0x00};\n")

def main():
    parser = argparse.ArgumentParser(description='Convert JSON file to a C++ constexpr array.')
    parser.add_argument('input_file', type=str, help='The input JSON file')
    parser.add_argument('output_file', type=str, help='The output C++ header file')
    args = parser.parse_args()

    data = read_json(args.input_file)
    filtered_data = filter_json(data)
    print(filtered_data)
    write_cpp_header(filtered_data, args.output_file)

if __name__ == '__main__':
    main()