import argparse
import re
import subprocess

VERSION_FILE = "VERSION"


def update_sdk_version(version_content):
    # VERSION ファイルはバージョン番号のみを含む
    version_str = version_content.strip()

    version_match = re.match(r"(\d{4}\.\d+\.\d+)(-canary\.(\d+))?", version_str)
    if version_match:
        major_minor_patch = version_match.group(1)
        canary_suffix = version_match.group(2)
        if canary_suffix is None:
            new_version = f"{major_minor_patch}-canary.0"
        else:
            canary_number = int(version_match.group(3))
            new_version = f"{major_minor_patch}-canary.{canary_number + 1}"

        return new_version
    else:
        raise ValueError(f"Invalid version format in VERSION file: {version_str}")


def write_file(filename, updated_content, dry_run):
    # ファイルの末尾に改行を追加
    updated_content = updated_content.rstrip() + "\n"
    if dry_run:
        print(f"Dry run: The following changes would be written to {filename}:")
        print(updated_content)
    else:
        with open(filename, "w") as file:
            file.write(updated_content)
        print(f"{filename} updated.")


def git_operations(new_version, dry_run):
    if dry_run:
        print("Dry run: Would execute git commit -am '[canary] Update VERSION'")
        print(f"Dry run: Would execute git tag {new_version}")
        print("Dry run: Would execute git push")
        print(f"Dry run: Would execute git push origin {new_version}")
    else:
        print("Executing: git commit -am '[canary] Update VERSION'")
        subprocess.run(["git", "commit", "-am", "[canary] Update VERSION"], check=True)

        print(f"Executing: git tag {new_version}")
        subprocess.run(["git", "tag", new_version], check=True)

        print("Executing: git push")
        subprocess.run(["git", "push"], check=True)

        print(f"Executing: git push origin {new_version}")
        subprocess.run(["git", "push", "origin", new_version], check=True)


def main():
    parser = argparse.ArgumentParser(description="Update VERSION file and push changes with git.")
    parser.add_argument(
        "--dry-run", action="store_true", help="Perform a dry run without making any changes."
    )
    args = parser.parse_args()

    # Read and update the VERSION file
    with open(VERSION_FILE, "r") as file:
        version_content = file.read()
    new_version = update_sdk_version(version_content)
    write_file(VERSION_FILE, new_version, args.dry_run)

    # Perform git operations
    git_operations(new_version, args.dry_run)


if __name__ == "__main__":
    main()
