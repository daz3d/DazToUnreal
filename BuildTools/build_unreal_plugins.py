# build_unreal_plugin.py
import os
import sys
import subprocess

def print_usage():
    print("Usage: python build_unreal_plugins.py [UE_VERSION]")
    print("Supported UE_VERSION: UE425, UE426, UE427, UE50, UE51, UE52, UE53, UE54, UE55, UE56")
    print("If no version is specified, all versions will be built.")

engine_path_map = {
    "UE425": "C:/Epic Games/UE_4.25/Engine/",
    "UE426": "C:/Epic Games/UE_4.26/Engine/",
    "UE427": "C:/Epic Games/UE_4.27/Engine/",
    "UE50": "C:/Epic Games/UE_5.0/Engine/",
    "UE51": "C:/Epic Games/UE_5.1/Engine/",
    "UE52": "C:/Epic Games/UE_5.2/Engine/",
    "UE53": "C:/Epic Games/UE_5.3/Engine/",
    "UE54": "C:/Epic Games/UE_5.4/Engine/",
    "UE55": "C:/Epic Games/UE_5.5/Engine/",
    "UE56": "C:/Epic Games/UE_5.6/Engine/"
}

global runuat_path, plugin_path, package_path

def setup_paths(ue_version):
    global runuat_path, plugin_path, package_path
    engine_path = engine_path_map[ue_version]
    relative_runuat_path = "Build/BatchFiles/RunUAT.bat"
    runuat_path = os.path.join(engine_path, relative_runuat_path)
    plugin_path  = "C:/Github/DazToUnreal-daz3d/UnrealPlugin/DazToUnreal/DazToUnreal.uplugin"
    package_path = f"C:/UE_DEPLOY/{ue_version}/DazToUnreal"

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
        "-set:GameConfigurations=Development;Shipping",
        # Optional: target platform(s). Uncomment if you want to constrain.
        # "-TargetPlatforms=Win64",
    ]

    print(f"DEBUG: args: {args}", flush=True)

    # Use shell=False to avoid cmd.exe/PowerShell parsing issues
    proc = subprocess.run(args, shell=False)

    return proc.returncode

def main(argv):
    global package_path

    print("DEBUG: argv:", argv)
    # parse argv to read ue_version string
    if len(argv) == 0:
        target_version = "all"
    elif len(argv) > 0:
        target_version = argv[0]

    if target_version.lower() == "all":
        ue_version_list = ["UE425", "UE426", "UE427", "UE50", "UE51", "UE52", "UE53", "UE54", "UE55", "UE56"]
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

    if success_list:
        print(f"Successfully built plugins for: {success_list}")
    if fail_list:
        print(f"Failed to build plugins for: {fail_list}")
 
    return 0

if __name__ == "__main__":
    main(sys.argv[1:])
    print("Script complete.")
