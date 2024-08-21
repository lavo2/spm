#include <cstdio>
#include <cstring>
#include <string>
#include <mpi.h>
#include <utilitympi.hpp>

// ------------------------------------------------------------------------------------------

static inline void usage(const char *argv0) {
    std::printf("--------------------\n");
    std::printf("Usage: %s [options] file-or-directory [file-or-directory]\n",argv0);
    std::printf("\nOptions:\n");
    std::printf(" -t set the \"BIG file\" low threshold (in Mbyte -- min. and default %ld Mbyte)\n",BIGFILE_LOW_THRESHOLD/(1024*1024));
    std::printf(" -r 0 does not recur, 1 will process the content of all subdirectories (default r=0)\n");
    std::printf(" -C compress: 0 preserves, 1 removes the original file (default C=0)\n");
    std::printf(" -D decompress: 0 preserves, 1 removes the original file\n");
    std::printf(" -q 0 silent mode, 1 prints only error messages to stderr, 2 verbose (default q=1)\n");
	std::printf(" -v 0 normal, 1 verbose for debugging\n");
    std::printf("--------------------\n");
}

int parseCommandLine(int argc, char *argv[], int rank) {
    extern char *optarg;
    const std::string optstr = "t:r:C:D:q:v:";

    long opt, start = 1;
    bool cpresent = false, dpresent = false;
    while ((opt = getopt(argc, argv, optstr.c_str())) != -1) {
        switch (opt) {
            case 't': {
                long t = 0;
                if (!isNumber(optarg, t)) {
                    std::fprintf(stderr, "Error: wrong '-t' option\n");
                    usage(argv[0]);
                    return -1;
                }
                t = std::min(2l, t);
                BIGFILE_LOW_THRESHOLD = t * (1024 * 1024);
                start += 2;
                if (BIGFILE_LOW_THRESHOLD > 100 * 1024 * 1024) { // just to set a limit
                    std::fprintf(stderr, "Error: \"BIG file\" low threshold too high %ld\n", BIGFILE_LOW_THRESHOLD);
                    return -1;
                }
            } break;
            case 'r': {
                long n = 0;
                if (!isNumber(optarg, n)) {
                    std::fprintf(stderr, "Error: wrong '-r' option\n");
                    usage(argv[0]);
                    return -1;
                }
                if (n == 1) RECUR = true;
                start += 2;
            } break;
            case 'C': {
                long c = 0;
                if (!isNumber(optarg, c)) {
                    std::fprintf(stderr, "Error: wrong '-C' option\n");
                    usage(argv[0]);
                    return -1;
                }
                cpresent = true;
                if (c == 1) REMOVE_ORIGIN = true;
                start += 2;
            } break;
            case 'D': {
                long d = 0;
                if (!isNumber(optarg, d)) {
                    std::fprintf(stderr, "Error: wrong '-D' option\n");
                    usage(argv[0]);
                    return -1;
                }
                dpresent = true;
                if (d == 1) REMOVE_ORIGIN = true;
                comp = false;
                start += 2;
            } break;
            case 'q': {
                long q = 0;
                if (!isNumber(optarg, q)) {
                    std::fprintf(stderr, "Error: wrong '-q' option\n");
                    usage(argv[0]);
                    return -1;
                }
                QUITE_MODE = q;
                start += 2;
            } break;
            case 'v': {
                long v = 0;
                if (!isNumber(optarg, v)) {
                    std::fprintf(stderr, "Error: wrong '-v' option\n");
                    usage(argv[0]);
                    return -1;
                }
                VERBOSE = v;
                start += 2;
            } break;
            default:
                usage(argv[0]);
                return -1;
        }
    }
    if (cpresent && dpresent) {
        std::fprintf(stderr, "Error: -C and -D are mutually exclusive!\n");
        usage(argv[0]);
        return -1;
    }
    if ((argc - start) <= 0) {
        if (!rank) std::fprintf(stderr, "Error: at least one file or directory should be provided!\n");
        if (!rank) usage(argv[0]);
        return -1;
    }
    return start;
}