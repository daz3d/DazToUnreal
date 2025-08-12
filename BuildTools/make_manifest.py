

import sys, os

def print_usage():
    print("Usage: python3 make_manifest.py <source_path> <output_file>")
    return

def walk_dir(source_dir):
    file_list = []
    print("DEBUG: walking source_dir: ", source_dir)
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            absolute_path = os.path.join(root, file)
            relative_path = os.path.relpath(os.path.join(root, file), source_dir)
            file_list.append(absolute_path)
    return file_list

def main(args):
    # print("DEBUG: args: ", args)

    if args == []:
        print_usage()
        return False

    try:
        source_path = args[0]
        output_file = args[1]
    except Exception as e:
        print("ERROR reading command-line: ", e)
        print_usage()
        return False

    print("DEBUG: source_path: ", source_path)
    print("DEBUG: output_file: ", output_file)

    source_dir = os.path.abspath(source_path)
    if os.path.exists(source_dir) == False:
        print("ERROR: source directory does not exist.")
        return False
    if os.path.isdir(source_dir) == False:
        print("ERROR: source is not a directory.")
        return False

    output_file_path = os.path.abspath(output_file)
    output_folder = os.path.dirname(output_file_path)
    if os.path.exists(output_folder) == False:
        print("ERROR: output folder does not exist.")
        return False
    if os.path.isdir(output_folder) == False:
        print("ERROR: output is not a directory.")
        return False

    file_list = walk_dir(source_dir)
    print("DEBUG: file_list: ", file_list[0:5])

    # check if output_file is a valid filename
    print("DEBUG: output_folder: ", output_folder)
    if os.path.abspath(output_file) == os.path.abspath(output_folder):
        print("ERROR: invalid output filename: ", output_file)
        return False
    # write file_list to output_file
    with open(output_file, 'w', encoding="utf-8") as f:
        for file in file_list:
            f.write(file + '\n')

    return True

if __name__ == '__main__':
    # print("DEBUG: sys_args: ", sys.argv)
    return_val = main(sys.argv[1:])
    if return_val:
        print("Done.")