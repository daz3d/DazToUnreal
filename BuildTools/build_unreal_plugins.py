# build_unreal_plugin.py
import os
import sys
import subprocess
import json

global plugin_path, output_path, runuat_path, package_path

def print_usage():
    print("Usage: python build_unreal_plugins.py [UE_VERSION]")
    print("Supported UE_VERSION: UE425, UE426, UE427, UE50, UE51, UE52, UE53, UE54, UE55, UE56")
    print("If no version is specified, all versions will be built.")

def is_windows():
    return sys.platform.startswith('win')
def is_mac():
    return sys.platform.startswith('darwin')

def load_config():
    global plugin_path, output_path, engine_path_map, compiler_path_map
    if is_windows():
        config_path = "build_unreal_plugins_win.json"
    elif is_mac():
        config_path = "build_unreal_plugins_mac.json"
    else:
        print("ERROR: Unsupported platform")
        sys.exit(1)
    with open(config_path, "r") as f:
        config = json.load(f)
        plugin_path = config["plugin_path"]
        output_path = config["output_path"]
        engine_path_map = config["engine_path_map"]
        compiler_path_map = config["compiler_path_map"]

def switch_msvc_version(msvc_ver):
    global compiler_path_map
    if msvc_ver not in compiler_path_map or compiler_path_map[msvc_ver] == "":
        print(f"ERROR: Unsupported MSVC version: {msvc_ver}")
        return False
    msvc_path = compiler_path_map[msvc_ver]
    pass
    return True

def reset_msvc_version():
    pass

def switch_xcode_version(xcode_ver):
    global compiler_path_map
    if xcode_ver not in compiler_path_map or compiler_path_map[xcode_ver] == "":
        print(f"ERROR: Unsupported Xcode version: {xcode_ver}")
        return False
    xcode_path = compiler_path_map[xcode_ver]
    # os.system(f"sudo xcode-select -s {xcode_path}")
    # set DEVELOPER_DIR
    os.environ["DEVELOPER_DIR"] = xcode_path
    return True

def reset_xcode_version():
    # os.system("sudo xcode-select -s /Applications/Xcode.app/Contents/Developer")
    # reset DEVELOPER_DIR
    os.environ["DEVELOPER_DIR"] = "/Applications/Xcode.app/Contents/Developer"

def setup_paths(ue_version):
    global runuat_path, plugin_path, package_path
    engine_path = engine_path_map[ue_version]
    relative_runuat_path = "Build/BatchFiles/RunUAT.bat"
    runuat_path = os.path.join(engine_path, relative_runuat_path)
    if not os.path.exists(runuat_path):
        runuat_path = runuat_path.replace(".bat", ".sh")
    package_path = f"{output_path}/{ue_version}/DazToUnreal"

def build_plugin(ue_version):
    global runuat_path, plugin_path, package_path
    setup_paths(ue_version)

    # Basic validation so we fail fast with a clear message
    if not os.path.exists(runuat_path):
        print(f"ERROR: RunUAT not found: {runuat_path}")
        return 1
    if not os.path.exists(plugin_path):
        print(f"ERROR: .uplugin not found: {plugin_path}")
        return 1
    os.makedirs(package_path, exist_ok=True)

    # Build argument vector (no shell quoting needed)
    args = [
        str(runuat_path),
        "BuildPlugin",
        f"-Plugin={plugin_path}",
        f"-Package={package_path}",
        "-Rocket",
        "-set:GameConfigurations=Development;Shipping"
    ]

    if is_windows():
        if ue_version.startswith("UE4"):
            if not switch_msvc_version("v141"):
                return 1
        if ue_version.startswith("UE5"):
            if ue_version == "UE50" or ue_version == "UE51":
                if not switch_msvc_version("v141"):
                    return 1
            if ue_version == "UE52":
                if not switch_msvc_version("v142"):
                    return 1
            else:
                if not switch_msvc_version("v143"):
                    return 1

    if is_mac():
        if ue_version.startswith("UE4"):
            if not switch_xcode_version("13.2.1"):
                return 1
        if ue_version.startswith("UE5"):
            if ue_version== "UE50":
                if not switch_xcode_version("13.2.1"):
                    return 1
            elif ue_version == "UE51":
                if not switch_xcode_version("13.4.1"):
                    return 1
                args += ['-VeryVerbose']
            elif ue_version == "UE56":
                if not switch_xcode_version("16.4"):
                    return 1
                args += ['-TargetPlatforms=Mac','-Architecture_Mac="arm64+x86_64"','-VeryVerbose']
            else:
                if not switch_xcode_version("15.3"):
                    return 1
                args += ['-TargetPlatforms=Mac','-Architecture_Mac="arm64+x86_64"']

    print(f"DEBUG: args: {args}", flush=True)

    # Use shell=False to avoid cmd.exe/PowerShell parsing issues
    proc = subprocess.run(args, shell=False)

    return proc.returncode

def main(argv):
    global package_path

    load_config()

    print("DEBUG: argv:", argv)
    # parse argv to read ue_version string
    if len(argv) == 0:
        target_version = "all"
    elif len(argv) > 0:
        target_version = argv[0]

    if target_version.lower() == "all":
        ue_version_list = ["UE425", "UE426", "UE427", "UE50", "UE51", "UE52", "UE53", "UE54", "UE55", "UE56", "UE57"]
    elif target_version not in engine_path_map:
        print_usage()
        return 1
    else:
        ue_version_list = [target_version]

    success_list = []
    fail_list = []

    for ue_version in ue_version_list:
        print("\n========================================================")
        print(f"Building plugin for Unreal Engine {ue_version}...")
        return_code = build_plugin(ue_version)
        if return_code != 0:
            print(f"ERROR: Failed to build plugin for Unreal Engine {ue_version}, return code={return_code}.\n")
            fail_list += [ue_version]
        else:
            print(f"Successfully built Unreal plugin to: {package_path}\n")
            success_list += [ue_version]

    if is_mac():
        reset_xcode_version()

    if success_list:
        print(f"Successfully built plugins for: {success_list}")
    if fail_list:
        print(f"Failed to build plugins for: {fail_list}")
 
    return 0

if __name__ == "__main__":
    main(sys.argv[1:])
    print("Script complete.")
