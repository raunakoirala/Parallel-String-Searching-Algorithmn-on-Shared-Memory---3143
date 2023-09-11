#include <iostream> // Input and output stream functionality

#include <vector> // Containers for storing data efficiently

#include <string> // String manipulation and storage

#include <fstream> // File input and output operations

#include <sstream> // String stream operations for parsing

#include <ctime> // Time functions for measuring execution time


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


class BloomFilter {
public:
    // Constructor
    BloomFilter(int size, int numHashFunctions) {

        // Initialise a bit array of 'size' bits, setting all bits to false
        bitArray.resize(size, false);

        // Store the number of hash functions to be used
        hashFunctions = numHashFunctions;
    }

    void insert(const std::string& item) {
        // Loop through the hash functions
        for (int i = 0; i < hashFunctions; i++) {

            // Calculate the hash value for the item and reduce it to fit within the bit array size
            size_t hash = JenkinsHash(item) % bitArray.size();

            // Set the corresponding bit in the bit array to true
            bitArray[hash] = true;
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
    // Variable to store the bit array
    std::vector<bool> bitArray;

    // Variable to store the number of hash functions
    int hashFunctions;
};

int main() {

    // Timer for overall execution time
    struct timespec startOverall, endOverall;
    clock_gettime(CLOCK_MONOTONIC, &startOverall);

    // Initialise the Bloom Filter
    BloomFilter filter(10000000, 4);

    // Read all the words from the text files and store them in an array
    std::vector<std::string> allWords;
    std::vector<std::string> fileNames = {"SHAKESPEARE.txt", "MOBY_DICK.txt", "LITTLE_WOMEN.txt"};

    struct timespec startReadFiles, endReadFiles;
    clock_gettime(CLOCK_MONOTONIC, &startReadFiles);

    for (const std::string& fileName : fileNames) {
        std::ifstream file(fileName);
        std::string word;

        while (file >> word) {
            allWords.push_back(word); // Store each word in the array
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &endReadFiles);
    double durationReadFiles = (endReadFiles.tv_sec - startReadFiles.tv_sec) + (endReadFiles.tv_nsec - startReadFiles.tv_nsec) * 1e-9;

    // Timer for overall insertion time
    struct timespec startInsert, endInsert;
    clock_gettime(CLOCK_MONOTONIC, &startInsert);

    // Insert all the words into the Bloom Filter
    for (const std::string& word : allWords) {
        filter.insert(word); // Insert each word into the Bloom Filter
    }

    // Stop the insertion timer
    clock_gettime(CLOCK_MONOTONIC, &endInsert);
    double durationInsert = (endInsert.tv_sec - startInsert.tv_sec) + (endInsert.tv_nsec - startInsert.tv_nsec) * 1e-9;

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
    struct timespec startQuery, endQuery;
    clock_gettime(CLOCK_MONOTONIC, &startQuery);


    int correctMatches = 0;        // Counter for correct matches
    int incorrectMatches = 0;      // Counter for incorrect matches

    // Loop through all the queries stored in allQueries
    for (const auto& query : allQueries) {

        // Use the query to check if 'query.first' exists in the filter which is just the word in the query file
        bool exists = filter.query(query.first);

        // Check if the expected existence status (query.second) matches the result (exists), this is the number next to the word
        if ((query.second == 1 && exists) || (query.second == 0 && !exists)) {
            correctMatches++;
        } else {
            incorrectMatches++;
        }
    }

    // Stop the query timer
    clock_gettime(CLOCK_MONOTONIC, &endQuery);
    double durationQuery = (endQuery.tv_sec - startQuery.tv_sec) + (endQuery.tv_nsec - startQuery.tv_nsec) * 1e-9;

    // Stop the overall execution timer
    clock_gettime(CLOCK_MONOTONIC, &endOverall);
    double durationOverall = (endOverall.tv_sec - startOverall.tv_sec) + (endOverall.tv_nsec - startOverall.tv_nsec) * 1e-9;

    // Print the overall insertion, query, read and execution times in seconds
    std::cout << "Overall Insertion Time: " << durationInsert << " s" << std::endl;
    std::cout << "Overall Query Time: " << durationQuery << " s" << std::endl;
    std::cout << "Overall Read Files Time: " << durationReadFiles << " s" << std::endl;
    std::cout << "Overall Execution Time: " << durationOverall << " s" << std::endl;

    // Print the accuracy
    std::cout << "Correct Matches: " << correctMatches << std::endl;
    std::cout << "Incorrect Matches: " << incorrectMatches << std::endl;

    return 0;
}
