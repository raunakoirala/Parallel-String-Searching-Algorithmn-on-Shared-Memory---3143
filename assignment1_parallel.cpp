#include <iostream> // Input and output stream functionality

#include <vector> // Containers for storing data efficiently

#include <string> // String manipulation and storage

#include <fstream> // File input and output operations

#include <sstream> // String stream operations for parsing

#include <ctime> // Time functions for measuring execution time

#include <omp.h> // OpenMP header for parallel programming

#include <functional> // Header for the std::function utility


#define NUM_THREAD 4

// Jenkins Hash implementation

// Define a function called JenkinsHash that takes a constant reference to a string as input

uint32_t JenkinsHash(const std::string& key) {
    // Create a 32-bit hash value to 0
    uint32_t hash = 0;

    // Loop through each character in the input string
    for (size_t i = 0; i < key.length(); ++i) {
        // Add the ASCII value of the current character to the hash value
        hash += key[i];

        // Bitwise left shift the hash value by 10 bits and add it to itself
        hash += (hash << 10);

        // Bitwise exclusive OR (XOR) the hash value by shifting it right by 6 bits
        hash ^= (hash >> 6);
    }

    // Bitwise left shift the hash value by 3 bits and add it to itself
    hash += (hash << 3);

    // Bitwise exclusive OR (XOR) the hash value by shifting it right by 11 bits
    hash ^= (hash >> 11);

    // Bitwise left shift the hash value by 15 bits and add it to itself
    hash += (hash << 15);

    // Return the final hash value
    return hash;
}


// Function to measure the execution time of a given operation
double measureExecutionTime(const std::function<void()>& operation) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    operation();
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
}

class BloomFilter {
public:
    BloomFilter(int size, int numHashFunctions) {
        // Initialise a bit array, size is the no. of bits
        bitArray.resize(size, false);
        hashFunctions = numHashFunctions;
    }

    void parallelInsert(const std::vector<std::string>& items) {
        // Start a parallel region using OpenMP
        #pragma omp parallel
        {
            // Get ID of the current thread
            int thread_id = omp_get_thread_num();
            // Get the total no. of threads in the parallel region
            int numThreads = omp_get_num_threads();
            // Calculate the chunk size for dividing the work among threads
            int chunkSize = items.size() / numThreads;
            // Calculate the starting index for the current thread
            int startIndex = thread_id * chunkSize;
            // Calculate the ending index for the current thread
            int endIndex = (thread_id == numThreads - 1) ? items.size() : startIndex + chunkSize;

            // Iterate over a chunk of items assigned to this thread
            for (int i = startIndex; i < endIndex; i++) {
                // Perform multiple hash function evaluations for each item
                for (int j = 0; j < hashFunctions; j++) {
                    // Calculate the hash value for the current item
                    size_t hash = JenkinsHash(items[i]) % bitArray.size();

                    // Set the corresponding bit in the bit array to true
                    bitArray[hash] = true;
                }
            }
        }
    }

    bool query(const std::string& item) {
        // Loop through the hash functions
        for (int i = 0; i < hashFunctions; i++) {
            // Calculate the hash value for the item and reduce it to fit within the bit array size
            size_t hash = JenkinsHash(item) % bitArray.size();

            // Check if the corresponding bit in the bit array is false
            if (!bitArray[hash]) {
                return false; // Item is definitely not present
            }
        }
        return true; // Item is possibly present
    }

private:
    std::vector<bool> bitArray;
    int hashFunctions;
};

int main() {
    omp_set_num_threads(NUM_THREAD);

    // Timer for overall execution time
    struct timespec startOverall, endOverall;
    clock_gettime(CLOCK_MONOTONIC, &startOverall);

    // Initialise the Bloom Filter
    BloomFilter filter(10000000, 4);

    // Read all the words from the text files and store them in an array
    std::vector<std::string> allWords;
    std::vector<std::string> fileNames = {"SHAKESPEARE.txt", "MOBY_DICK.txt", "LITTLE_WOMEN.txt"};

    
    double durationReadFiles = measureExecutionTime([&]() {

        // Start another parallel region using OpenMP
        #pragma omp parallel
        {
            // Create a vector to store words for each thread
            std::vector<std::string> words;

            // Distribute the work among threads using a parallel loop
            #pragma omp for
            for (int i = 0; i < fileNames.size(); i++) {
                // Get the current file name from the list
                const std::string& fileName = fileNames[i];
                // Open the file for reading
                std::ifstream file(fileName);
                std::string word;

                // Read words from the file and store them in the vector
                while (file >> word) {
                    words.push_back(word);
                }
            }
            // Combine the vectors into the shared vector 'allWords'
            #pragma omp critical
            {
                allWords.insert(allWords.end(), words.begin(), words.end());
            }
        } 
    }); 

    // Timer for overall insertion time
    double durationInsert = measureExecutionTime([&]() {
        // Parallel insertion of words into the Bloom Filter
        filter.parallelInsert(allWords);
    });

    // Read all the queries from the file and store them in an array
    std::vector<std::pair<std::string, int>> allQueries;
    std::ifstream queryFile("query.txt");
    std::string queryLine;

    while (std::getline(queryFile, queryLine)) {
        std::istringstream iss(queryLine);
        std::string queryWord;
        int shouldExist;

        if (iss >> queryWord >> shouldExist) {
            allQueries.push_back({queryWord, shouldExist}); // Store each query in the array
        }
    }

    // Timer for overall query time
    double durationQuery = measureExecutionTime([&]() {

        int correctMatches = 0;
        int incorrectMatches = 0;

        // Start a parallel loop
        #pragma omp parallel for reduction(+:correctMatches, incorrectMatches)

        // Perform addition reduction operation on correctMatches and incorrectMatches


        // Loop through all the queries stored in allQueries
        for (int i = 0; i < allQueries.size(); i++) {
             // Use the query to check if 'query.first' exists in the filter which is just the word in the query file
            bool exists = filter.query(allQueries[i].first);

            // Check if the expected existence status (query.second) matches the result (exists), this is the number next to the word
            if ((allQueries[i].second == 1 && exists) || (allQueries[i].second == 0 && !exists)) {
                correctMatches++;
            } else {
                incorrectMatches++;
            }
        } 

        // Print the summary of correct and incorrect matches
        std::cout << "Correct Matches: " << correctMatches << std::endl;
        std::cout << "Incorrect Matches: " << incorrectMatches << std::endl;
    });


    // Stop the overall execution timer
    clock_gettime(CLOCK_MONOTONIC, &endOverall);
    double durationOverall = (endOverall.tv_sec - startOverall.tv_sec) + (endOverall.tv_nsec - startOverall.tv_nsec) * 1e-9;

    // Print the overall insertion, query, read and execution times in seconds
    std::cout << "Overall Insertion Time: " << durationInsert << " s" << std::endl;
    std::cout << "Overall Query Time: " << durationQuery << " s" << std::endl;
    std::cout << "Overall Read Files Time: " << durationReadFiles << " s" << std::endl;
    std::cout << "Overall Execution Time: " << durationOverall << " s" << std::endl;

    return 0;
}
