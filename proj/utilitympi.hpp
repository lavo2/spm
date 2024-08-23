#if !defined _UTILITY_HPP_MPI
#define _UTILITY_HPP_MPI

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <dirent.h>

#include <ftw.h>

#include <algorithm>
#include <string>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <iostream>

#include <cmath> 
#include <cassert>


#include <mpi.h>



#include <miniz/miniz.h>


#define SUFFIX ".zip"
#define BUF_SIZE (1024 * 1024)

// global variables with their default values -------------------------------------------------
static bool comp = true;                      // by default, it compresses 
static size_t BIGFILE_LOW_THRESHOLD=2097152;  // 2Mbytes threshold 
static bool REMOVE_ORIGIN=false;              // Does it keep the origin file?
static int  QUITE_MODE=1; 					 // 0 silent, 1 error messages, 2 verbose
static int  VERBOSE=0;                     	// 0 normal, 1 verbose for debugging
static bool RECUR= false;                     // do we have to process the contents of subdirs?
// --------------------------------------------------------------------------------------------

// map the file pointed by filepath in memory
// if size is zero, it looks for file size
// if everything is ok, it returns the memory pointer ptr
static inline bool mapFile(const char fname[], size_t &size, unsigned char *&ptr) {
    // open input file.
    int fd = open(fname,O_RDONLY);
    if (fd<0) {
	if (QUITE_MODE>=1) {
	    perror("mapFile open");
	    std::fprintf(stderr, "Failed opening file %s\n", fname);
	}
	return false;
    }
    if (size==0) {
	struct stat s;
	if (fstat (fd, &s)) {
	    if (QUITE_MODE>=1) {
		perror("fstat");
		std::fprintf(stderr, "Failed to stat file %s\n", fname);
	    }
	    return false;
	}
	size=s.st_size;
    }

    // map all the file in memory
    ptr = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
	if (QUITE_MODE>=1) {
	    perror("mmap");
	    std::fprintf(stderr, "Failed to memory map file %s\n", fname);
	}
	return false;
    }
    close(fd);
    return true;
}
// unmap a previously memory-mapped file
static inline void unmapFile(unsigned char *ptr, size_t size) {
    if (munmap(ptr, size)<0) {
	if (QUITE_MODE>=1) {
	    perror("nummap");
	    std::fprintf(stderr, "Failed to unmap file\n");
	}
    }
}

// check if dir is '.' or '..'
static inline bool isdot(const char dir[]) {
  int l = strlen(dir);  
  if ( (l>0 && dir[l-1] == '.') ) return true;
  return false;
}

static bool isNumber(const char* s, long &n) {
    try {
		size_t e;
		n=std::stol(s, &e, 10);
		return e == strlen(s);
    } catch (const std::invalid_argument&) {
		return false;
    } catch (const std::out_of_range&) {
		return false;
    }
}


// Function to read the file content into a vector
bool readFile(const std::string& filename, std::vector<unsigned char>& buffer) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }
    return true;
}

struct FileData {
    char filename[256];
    size_t size;
    std::vector<unsigned char> data;

    // Constructor for easy initialization
    FileData(const char* fname, size_t s, std::vector<unsigned char> d) 
        : size(s), data(std::move(d)) {
        // Copy filename with a limit to ensure it doesn't exceed 255 characters
        std::strncpy(filename, fname, sizeof(filename) - 1);
        // Ensure null-termination
        filename[sizeof(filename) - 1] = '\0';
    }
};



static inline bool walkDirAndGetFiles(const char dname[], std::vector<FileData>& fileDataVec, const bool comp, std::string relativePath = "") {
    struct stat statbuf;

    if (stat(dname, &statbuf) == -1) {
        if (QUITE_MODE >= 1) {
            perror("stat");
            std::fprintf(stderr, "Error: stat %s\n", dname);
        }
        return false;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        // Process as a directory
        if (chdir(dname) == -1) {
            if (QUITE_MODE >= 1) {
                perror("chdir");
                std::fprintf(stderr, "Error: chdir %s\n", dname);
            }
            return false;
        }

        DIR* dir;
        if ((dir = opendir(".")) == NULL) {
            if (QUITE_MODE >= 1) {
                perror("opendir");
                std::fprintf(stderr, "Error: opendir %s\n", dname);
            }
            return false;
        }

        struct dirent* file;
        bool error = false;
        while ((errno = 0, file = readdir(dir)) != NULL) {
            if (stat(file->d_name, &statbuf) == -1) {
                if (QUITE_MODE >= 1) {
                    perror("stat");
                    std::fprintf(stderr, "Error: stat %s\n", file->d_name);
                }
                return false;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                if (!isdot(file->d_name)) {
                    // Update relative path for the directory
                    std::string newRelativePath = relativePath + file->d_name + "/";
                    if (walkDirAndGetFiles(file->d_name, fileDataVec, comp, newRelativePath)) {
                        if (chdir("..") == -1) {
                            perror("chdir");
                            std::fprintf(stderr, "Error: chdir ..\n");
                            error = true;
                        }
                    } else {
                        error = true;
                    }
                }
            } else {
                std::string filename = file->d_name;
                size_t size = statbuf.st_size;

                // Check the file extension
                bool isZipFile = (filename.size() > 4 && filename.substr(filename.size() - 4) == ".zip");

                // Skip or include based on the 'comp' flag
                if ((comp && isZipFile) || (!comp && !isZipFile)) {
                    std::fprintf(stderr, "ignoring %s file %s\n", comp ? "compressed" : "non-compressed", file->d_name);
                    continue; // Skip if the conditions do not match
                }

                std::vector<unsigned char> data;
                if (!readFile(file->d_name, data)) return false;

                // Create a FileData object with the new order and store it in the vector
                fileDataVec.emplace_back((relativePath + file->d_name).c_str(), size, std::move(data));
            }
        }

        if (errno != 0) {
            if (QUITE_MODE >= 1) perror("readdir");
            error = true;
        }

        closedir(dir);
        return !error;

    } else {
        // Process as a single file
        std::string filename = dname;
        size_t size = statbuf.st_size;

        // Check the file extension
        bool isZipFile = (filename.size() > 4 && filename.substr(filename.size() - 4) == ".zip");

        // Skip or include based on the 'comp' flag
        if ((comp && isZipFile) || (!comp && !isZipFile)) {
            std::fprintf(stderr, "ignoring %s file %s\n", comp ? "compressed" : "non-compressed", dname);
            return true; // Skip if the conditions do not match
        }

        std::vector<unsigned char> data;
        if (!readFile(dname, data)) return false;

        // Create a FileData object with the new order and store it in the vector
        fileDataVec.emplace_back((relativePath + dname).c_str(), size, std::move(data));

        return true;
    }
}



struct InitialHeader {
    int numFiles;
    int sendCounts[128]; // Adjust size if needed, or dynamically allocate.
    int displs[128];     // Adjust size if needed, or dynamically allocate.
};


MPI_Datatype createIHeaderDataType(int size) {
    MPI_Datatype new_Type;
    MPI_Datatype old_types[3] = { MPI_INT, MPI_INT, MPI_INT };
    int blocklen[3] = { 1, size, size };

    MPI_Aint offsets[3];
    offsets[0] = offsetof(InitialHeader, numFiles);
    offsets[1] = offsetof(InitialHeader, sendCounts);
    offsets[2] = offsetof(InitialHeader, displs);

    MPI_Type_create_struct(3, blocklen, offsets, old_types, &new_Type);
    MPI_Type_commit(&new_Type);

    return new_Type;
}



struct ReceiveFiles {
    char filename[256];
    size_t size;
	unsigned long counts;
	unsigned char* myData = nullptr;
};

bool decompressBlock(unsigned char* input, size_t inputSize, unsigned char* output, size_t& outputSize) {
		int err;
		if ((err = uncompress(output, &outputSize, input, inputSize)) != Z_OK) {
			std::cerr << "Failed to decompress block, error: " << err << std::endl;
			return false;
		}
		return true;
	}

struct DataRec {
    char filename[256];
    size_t size;
    size_t nblock = 1;
    size_t blockid = 0;
    size_t lastblocksize = 0;
    std::vector<unsigned char*> recDataVec;

};

bool mergeAndZip(const std::vector<DataRec>& dataRecVec, const size_t lastblocksize) {
    std::string outfiles = dataRecVec[0].filename;
    std::string outfile = outfiles + SUFFIX;
    std::ofstream outFile(outfile, std::ios::binary);

    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return false;
    }
    //write the number of blocks
    size_t numBlocks = dataRecVec[0].nblock;
    outFile.write(reinterpret_cast<const char*>(&numBlocks), sizeof(size_t));

    //for each block, write the size of the block
    for (size_t i = 0; i < numBlocks; i++) {
        size_t size = dataRecVec[i].size;
        outFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    }
    //write last block size for decompression
    outFile.write(reinterpret_cast<const char*>(&lastblocksize), sizeof(size_t));

    //write all the data
    for (size_t i = 0; i < numBlocks; i++) {
        outFile.write(reinterpret_cast<const char*>(dataRecVec[i].recDataVec[0]), dataRecVec[i].size);
    }

    outFile.close();
    if (REMOVE_ORIGIN) {
        unlink(outfiles.c_str());
    }

    outFile.close();
    return true;

}

bool mergeAndWrite(const std::vector<DataRec>& dataRecVec) {
    std::string outfiles = dataRecVec[0].filename;
    std::string outfile = outfiles.substr(0, outfiles.size() - 4);

    std::ofstream outFile(outfile, std::ios::binary);
	if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return false;
	}
    size_t numBlocks = dataRecVec[0].nblock;

    for (size_t i = 0; i < numBlocks; i++) {
        outFile.write(reinterpret_cast<const char*>(dataRecVec[i].recDataVec[0]), dataRecVec[i].size);
    }

    if (REMOVE_ORIGIN) {
        unlink(outfiles.c_str());
    }
    outFile.close();

    return true;

}
    

#endif 