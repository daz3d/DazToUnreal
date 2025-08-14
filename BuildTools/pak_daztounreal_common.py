


import sys, os
from subprocess import list2cmdline

ue_path = r"C:/Epic Games/UE_4.25/"
platform = "Win64"
unreal_pak_executable = os.path.join(ue_path, "Engine/Binaries/", platform, "UnrealPak").replace("\\", "/")
if "win" in platform.lower():
    unreal_pak_executable += ".exe"

def print_usage():
    print("Usage: python3 pak_daztounreal_common.py <manifest_file> <manifest_common_root> <output_pakfile>")
    return

def main(args):
    if args == []:
        print_usage()
        return False

    try:
        manifest_file = args[0]
        common_root = args[1]
        output_pakfile = args[2]
    except Exception as e:
        print("ERROR reading command-line: ", e)
        print_usage()
        return False

    manifest_file_path = os.path.abspath(manifest_file)
    if not os.path.exists(manifest_file_path):
        print("ERROR: manifest file does not exist.")
        return False

    common_root = os.path.abspath(common_root).replace("\\", "/")
    if not os.path.exists(common_root):
        print("ERROR: common root folder does not exist.")
        return False
    if not os.path.isdir(common_root):
        print("ERROR: common root is not a folder.")
        return False

    output_pakfile_path = os.path.abspath(output_pakfile).replace("\\", "/")
    output_folder = os.path.dirname(output_pakfile_path)
    if not os.path.exists(output_folder):
        print("ERROR: output folder does not exist.")
        return False
    
    f = open(manifest_file_path, 'r')
    lines = f.readlines()
    f.close()

    if not lines:
        print("ERROR: manifest file is empty.")
        return False
    
    uepak_content_lines = []
    for l in lines:
        l = l.strip().replace("\\", "/")
        if not l:
            continue
        if not os.path.exists(l):
            print(f"WARNING: file {l} does not exist, skipping...")
            continue
        relative_path = l[l.lower().find(common_root.lower())+len(common_root):]
        if relative_path.startswith("/"):
            relative_path = relative_path[1:]
        internal_path = "../../../Game/DazToUnreal/Common/" + relative_path
        uepak_content_line = f"{l} {internal_path}\n"
        uepak_content_lines.append(uepak_content_line)
    
    if not uepak_content_lines:
        print("ERROR: no valid content files found in manifest.")
        return False
    
    pakfile_basename = os.path.basename(output_pakfile_path)
    pak_content_file = os.path.join(output_folder, "pak_content_" + pakfile_basename + ".txt").replace("\\", "/")
    with open(pak_content_file, 'w') as f:
        f.writelines(uepak_content_lines)
    
    print(f"DEBUG: manifest_file_path: {manifest_file_path}")
    print(f"DEBUG: common_root: {common_root}")
    print(f"DEBUG: output_pakfile_path: {output_pakfile_path}")
    print(f"DEBUG: pak_content_file: {pak_content_file}")

    # Call the UnrealPak command to create the pak file
    # command = f'"{unreal_pak_executable}" "{output_pakfile_path}" -create="{pak_content_file}"'
    # print(f"DEBUG: command: {command}")

    args = [
        unreal_pak_executable,
        output_pakfile_path,
        "-create=" + pak_content_file
    ]
    command = list2cmdline(args)
    print(f"DEBUG: command: {command}")
    result = os.system(command)
    if result != 0:
        print(f"ERROR: UnrealPak command failed with exit code {result}.")
        return False
    print(f"Successfully created pak file: {output_pakfile_path}")

if __name__ == "__main__":
    main(sys.argv[1:])