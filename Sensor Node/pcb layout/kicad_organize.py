import os
import shutil

# Define the working directory (current directory)
source_dir = os.getcwd()

# Define mapping of extensions to target folders
extension_to_folder = {
    '.kicad_sym': 'symbols',
    '.kicad_mod': 'footprints',
    '.step': 'models',
    '.stp': 'models'
}

# Extensions of files to delete
delete_extensions = {'.htm', '.txt'}

# Ensure destination folders exist
for folder in extension_to_folder.values():
    os.makedirs(os.path.join(source_dir, folder), exist_ok=True)

# Process each file in the directory
for filename in os.listdir(source_dir):
    file_path = os.path.join(source_dir, filename)

    if os.path.isfile(file_path):
        _, ext = os.path.splitext(filename)

        # Delete unwanted files
        if ext in delete_extensions:
            print(f"Deleting {filename}")
            os.remove(file_path)

        # Move files to appropriate folders
        elif ext in extension_to_folder:
            dest_folder = os.path.join(source_dir, extension_to_folder[ext])
            dest_path = os.path.join(dest_folder, filename)

            # If the file exists in destination, overwrite it
            if os.path.exists(dest_path):
                print(f"Overwriting existing file: {filename}")
                os.remove(dest_path)

            print(f"Moving {filename} -> {extension_to_folder[ext]}/")
            shutil.move(file_path, dest_path)
