o compress

./mainseq -C 1 -r 1 -q 2 "/Users/lavo/Desktop/Projects/spm/proj2/dataset/small"  
./mainseq -D 1 -r 1 -q 2 "/Users/lavo/Desktop/Projects/spm/proj2/dataset/small"  

Note that without -r 1 we need to select a specific file, for example;

./mainseq -C 1 -r 1 -q 2 "/Users/lavo/Desktop/Projects/spm/proj2/dataset/small/file_1KB.txt" 
./mainseq -D 1 -r 1 -q 2 "/Users/lavo/Desktop/Projects/spm/proj2/dataset/small/file_1KB.txt.zip" 

Usage: ./ffc_farm [options] file-or-directory [file-or-directory]

Options:
 -n set the n. of Workers (default nworkers=8)
 -t set the "BIG file" low threshold (in Mbyte -- min. and default 2 Mbyte)
 -r 0 does not recur, 1 will process the content of all subdirectories (default r=0)
 -C compress: 0 preserves, 1 removes the original file (default C=0)
 -D decompress: 0 preserves, 1 removes the original file
 -q 0 silent mode, 1 prints only error messages to stderr, 2 verbose (default q=1)
 -a asynchrony degree for the on-demand policy (default a=1)
 -b 0 blocking, 1 non-blocking concurrency control (default b=0)

# TODO
check origialn file pointer deallocation (if already done eliminate filePtr from task)
check memory leaks
CHANGE ALL READ AND WRITE FILE WITH ONE IN UTILITY.HPP (maybe)


FIX single block decompression
MAYBE LOCK FOR FF BACKSTUFF