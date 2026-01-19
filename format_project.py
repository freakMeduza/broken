import os
import subprocess
import sys

# Configuration: mapping file extensions to formatting commands
# -i flag stands for "in-place" (overwrites the file)
FORMATTING_CONFIG = {
    "cmake": {
        "extensions": [".cmake", "CMakeLists.txt"],
        "command": ["cmake-format", "-i"]
    },
    "cpp": {
        "extensions": [".cpp", ".h", ".hpp", ".c", ".cxx", ".cc"],
        "command": ["clang-format", "-i", "-style=file"]
    },
    "glsl": {
        "extensions": [".glsl", ".vert", ".frag", ".geom", ".comp", ".tesc", ".tese"],
        "command": ["clang-format", "-i", "-style=file"]
    }
}

# Directories to ignore
EXCLUDE_DIRS = {"build", ".git", ".vscode", "vcpkg_installed", "node_modules", "bin"}

def run_formatter(command, file_path):
    try:
        subprocess.run(command + [file_path], check=True, capture_output=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: File '{file_path}': {e.stderr.decode().strip()}")
    except FileNotFoundError:
        print(f"Error: Command '{command[0]}': not found")
        sys.exit(1)
    return False

def format_project(target_directory):
    print(f"Starting formatting in: '{os.path.abspath(target_directory)}'")
    
    count = 0
    for root, dirs, files in os.walk(target_directory):
        # Filter out excluded directories
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]

        for file in files:
            file_path = os.path.join(root, file)
            
            for tool, config in FORMATTING_CONFIG.items():
                if any(file.endswith(ext) or file == ext for ext in config["extensions"]):
                    if run_formatter(config["command"], file_path):
                        print(f"Formatted [{tool}]: {file_path}")
                        count += 1
    
    print(f"\nFormatting finished. Total files processed: {count}")

if __name__ == "__main__":
    # Default to current directory or use first argument
    project_path = sys.argv[1] if len(sys.argv) > 1 else "."
    format_project(project_path)
