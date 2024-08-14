import os

def delete_zip_files(directory):
    # Check if the directory exists
    if not os.path.exists(directory):
        print(f"The directory {directory} does not exist.")
        return
    
    # List all files in the directory
    files_in_directory = os.listdir(directory)
    
    # Filter out only .zip files
    zip_files = [file for file in files_in_directory if file.endswith('.zip')]
    
    # Delete each .zip file
    for zip_file in zip_files:
        file_path = os.path.join(directory, zip_file)
        os.remove(file_path)
        print(f"Deleted: {zip_file}")
    
    if not zip_files:
        print("No .zip files found.")
    else:
        print("All .zip files have been deleted.")

def main():
    # Specify the directory where you want to delete .zip files
    directory = "../dataset/small"
    
    # Call the delete function
    delete_zip_files(directory)

if __name__ == "__main__":
    main()
