import os
import random
import string

# Function to generate random text
def generate_random_text(size_in_bytes):
    chars = string.ascii_letters + string.digits + string.punctuation + ' \n'
    return ''.join(random.choice(chars) for _ in range(size_in_bytes))

# Function to create a file of a specific size
def create_text_file(file_path, size_in_bytes):
    with open(file_path, 'w') as file:
        text = generate_random_text(size_in_bytes)
        file.write(text)

# Function to create multiple files with varying sizes
def generate_dataset(directory, sizes_in_kb):
    if not os.path.exists(directory):
        os.makedirs(directory)
    
    for size in sizes_in_kb:
        file_name = f"file_{size}KB.txt"
        file_path = os.path.join(directory, file_name)
        create_text_file(file_path, size * 1024)
        print(f"Created {file_name} of size {size}KB")

# Main function to define sizes and call the generation functions
def main():
    # Sizes in KB (for 1GB use 1024 * 1024 KB)
    sizes_in_kb = [4 * 20 * 1024, 4 * 22 * 1024, 4 * 24 * 1024, 4 * 26 * 1024, 4 * 28 * 1024, 4 * 29 * 1024]
    
    # Directory to save the dataset
    directory = "text_files_dataset"
    
    # Generate dataset
    generate_dataset(directory, sizes_in_kb)
    print("Dataset generation complete!")

if __name__ == "__main__":
    main()
