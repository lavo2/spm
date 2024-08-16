def compare_files(file1_path, file2_path):
    try:
        with open(file1_path, 'r') as file1, open(file2_path, 'r') as file2:
            file1_content = file1.read()
            file2_content = file2.read()

            if file1_content == file2_content:
                print("The files are equal.")
            else:
                print("The files are not equal.")
    except FileNotFoundError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

# Example usage
compare_files('/Users/lavo/Desktop/Projects/spm/proj2/dataset/test/file_500KB.txt', 
              '/Users/lavo/Desktop/Projects/spm/proj2/dataset/small/file_500KB.txt')
