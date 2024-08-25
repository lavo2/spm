def compare_files(file1_path, file2_path):
    try:
        with open(file1_path, 'r') as file1, open(file2_path, 'r') as file2:
            file1_content = file1.read()
            file2_content = file2.read()
            # print the size of the two files
            print(f"File 1 size: {len(file1_content)} bytes")
            print(f"File 2 size: {len(file2_content)} bytes")

            if file1_content == file2_content:
                print("The files are equal.")
            else:
                print("The files are not equal.")
                # print the first difference
                for i, (line1, line2) in enumerate(zip(file1_content.splitlines(), file2_content.splitlines())):
                    if line1 != line2:
                        print(f"First difference found at line {i + 1}:")
                        print(f"File 1: {line1}")
                        print(f"File 2: {line2}")
                        # print the size of the two lines
                        print(f"Line 1 size: {len(line1)} bytes")
                        print(f"Line 2 size: {len(line2)} bytes")
                        break


    except FileNotFoundError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

# Example usage
compare_files('/Users/lavo/Desktop/Projects/spm/proj/dataset/test/medium/file_102400KB.txt', 
              '/Users/lavo/Desktop/Projects/spm/proj/dataset/medium/file_102400KB.txt')

#compare_files('/Users/lavo/Desktop/Projects/spm/proj/dataset/test/medium/file_10240KB.txt', 
              #'/Users/lavo/Desktop/Projects/spm/proj/dataset/medium/file_10240KB.txt')
