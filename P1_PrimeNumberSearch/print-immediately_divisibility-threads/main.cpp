#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <climits>
#include <algorithm>
#include <syncstream>
#include <chrono>
#include <format>

std::string stamp(){
    auto ms = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    return std::format("{:%Y-%m-%d %H:%M:%S}", ms);
}

void primecheck_job_scheme2(int id, int number, const std::vector<int>& divisors, std::vector<int> &results, int threads){
    int result;
    int display_id;
    for (const auto& div : divisors){
        if(number % div == 0 && number != div){
            break;
        }
        else if(div == divisors.back()){
            result = number;
            results[id] = std::move(result);
            if (threads == 0) display_id = 0; else display_id = id % threads;
            {
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::floor<std::chrono::milliseconds>(now);
                // compiling with mac libc++ clang needs '-fexperimental-library' flag. alternative is mutex
                std::osyncstream(std::cout) << "(" << std::format("{:%H:%M:%S}", ms) << ")" << " Thread ID: " << display_id << " found prime: " << number << "\n";
            }
        }
    }
} 


int main(){
    std::ifstream file("config.txt");
    if (!file.is_open()){
        std::cout << "Could not open config.txt." << std::endl;
        return 1;
    }
    std::string key;
    std::vector<std::thread> thread_list;
    int threads = 1, range = 0;
    long buffer = 0;

    // todo.. robust-er stuffs.
    while (file >> key){
        if (!(file >> buffer)){
            std::cout << "Invalid value for '" << key << "' in config.txt." << std::endl;
            return 1;
        }
        if (key == "range") {
            if (buffer < 0 || buffer > INT_MAX){
                range = -1;
            }
            else {
                range = buffer;
            }
        }
        else if (key == "threads") {
            if (buffer < 0 || buffer > INT_MAX){
                threads = -1;
            }
            else {
                threads = buffer;
            }
        }
    }

    if (threads < 0){
        std::cout << "Invalid thread value." << std::endl;
    }
    else if (range < 0){
        std::cout << "Invalid range value." << std::endl;
    }
    else if (range == 2){
        std::cout << "Start: " << stamp() << "\n";
        std::cout << "2" << std::endl;
        std::cout << "End: " << stamp() << "\n";
    }
    else if (range == 3){
        std::cout << "Start: " << stamp() << "\n";
        std::cout << "2 3" << std::endl;
        std::cout << "End: " << stamp() << "\n";
    }
    else {
        int id = 0;
        std::vector<int> results(range);
        std::vector<int> divisors;
        thread_list.reserve(threads);

        for(int i = 2; i <= range / i; i++){
            divisors.push_back(i);
        }
        std::cout << "Start: " << stamp() << "\n";
        // start the timer
        auto start = std::chrono::steady_clock::now();

        // 0 threads: just run on main thread
        if (threads == 0){
            for (int j = 2; j <= range; j++) primecheck_job_scheme2(id++, j, divisors, results, threads);
        }

        for(int i = 2; threads > 0 && i <= range; i += threads){
            if(i + threads > range){
                for (int j = i; j <= range; j++){
                    thread_list.emplace_back(primecheck_job_scheme2, id++, j, std::cref(divisors), std::ref(results), threads);
                }
                for (auto& th : thread_list) th.join();
            }
            else {
                for (int j = i; j < i + threads; j++){
                    thread_list.emplace_back(primecheck_job_scheme2, id++, j, std::cref(divisors), std::ref(results), threads);
                }
                for (auto& th : thread_list) th.join();
            }
            thread_list.clear();
        }

        // stop the clock
        auto end = std::chrono::steady_clock::now();
        std::string end_stamp = stamp();
        auto time_elapsed = end - start;
        std::chrono::duration<double, std::milli> ms = end - start;
        std::cout << "End: " << end_stamp << "\n";
        std::cout << "Elapsed: " << ms.count() << " ms\n";

        // the finale
        // excluded cus i think not needed..?
        /*
        size_t total = 0;
        std::sort(results.begin(), results.end());
        for (const auto& r : results){
            if (r != 0){
                std::cout << r << " ";
            }
        }
        */
       std::cout << std::endl;
    }
    return 0;
}