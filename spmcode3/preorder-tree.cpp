#include <iostream>
#include "threadPool.hpp"

int main () {
    ThreadPool TP(4); // a thread pool with 4 Workers

    // function to be processed by Worker threads
    auto square = [](const uint64_t x) {
        return x*x;
    };

    const uint64_t num_nodes = 13;
    std::vector<std::future<uint64_t>> futures;
    // preorder binary tree traversal
    typedef std::function<void(uint64_t)> traverse_t;
    traverse_t traverse = [&] (uint64_t node){
        if (node < num_nodes) {
            // submit the job
            auto future = TP.enqueue(square, node);
            futures.emplace_back(std::move(future));

            // traverse a complete binary tree
            traverse(2*node+1); // left child
            traverse(2*node+2); // right child
        }
    };

    // start at the root node
    traverse(0);

    // get the results
    for (auto& future : futures)
        std::cout << future.get() << std::endl;
}

