import os
import subprocess

EXTENSIONS = (".cpp", ".hpp", ".hpp.in", ".comp")
EXCLUSIONS = {"build", ".git", ".vscode"}

def format_sources(root_dir="."):
    subprocess.run(["clang-format", "--version"], check=True)

    for root, dirs, files in os.walk(root_dir):
        dirs[:] = [dir for dir in dirs if dir not in EXCLUSIONS]

        for file in files:
            if file.endswith(EXTENSIONS):
                path = os.path.join(root, file)
                print(f"clang-format: {path}")
                try:
                    subprocess.run(["clang-format", "-i", "--style=file", path], check=True)
                except subprocess.CalledProcessError as error:
                    print(f"clang-format: {path} error: {error}")

if __name__ == "__main__":
    format_sources()