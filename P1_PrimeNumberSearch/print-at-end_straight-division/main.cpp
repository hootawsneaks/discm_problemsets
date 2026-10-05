#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <climits>
#include <algorithm>
#include <chrono>
#include <format>

std::string stamp(){
    auto ms = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    return std::format("{:%Y-%m-%d %H:%M:%S}", ms);
}

void primecheck_job_scheme1(int id, int range_begin, int range_end, const std::vector<int>& divisors, std::vector<std::vector<int>> &results){
    std::vector<int> result;
    for (int i = range_begin; i <= range_end; i++){
        for (const auto& div : divisors){
            if(i % div == 0 && i != div){
                break;
            }
            else if(div == divisors.back()){
                result.push_back(i);
            }
        }
    }
    results[id] = std::move(result);
} 


int main(){
    std::ifstream file("config.txt");
    std::string key;
    std::vector<std::thread> thread_list;
    int threads = 1, range = 0;
    long buffer = 0;

    // todo.. robust-er stuffs.
    while (file >> key){
        file >> buffer;
        if (key == "range") {
            if (buffer < 0 || buffer > INT_MAX){
                range = -1;
            }
            else {
                range = buffer;
            }
        }
        else if (key == "threads") {
            if (buffer < 1 || buffer > INT_MAX){
                threads = -1;
            }
            else {
                threads = buffer;
            }
        }
    }

    if (threads <= 0){
        std::cout << "Invalid thread value." << std::endl;
    }
    else if (range < 0){
        std::cout << "Invalid range value." << std::endl;
    }
    else if (range == 2){
        std::cout << "2" << std::endl;
    }
    else if (range == 3){
        std::cout << "2 3" << std::endl;
    }
    else {
        int segment = (range - 1) / threads;
        int id = 0;
        thread_list.reserve(threads);
        std::vector<std::vector<int>> results(threads);
        std::vector<int> divisors;

        for(int i = 2; i <= range / i; i++){
            divisors.push_back(i);
        }

        std::cout << "Start: " << stamp() << "\n";
        // start the timer
        auto start = std::chrono::steady_clock::now();
        for(int i = 2; i <= range; i += segment + 1){
            if(i + segment >= range){
                thread_list.emplace_back(primecheck_job_scheme1, id++, i, range, std::cref(divisors), std::ref(results));
            }
            else{
                thread_list.emplace_back(primecheck_job_scheme1, id++, i, i + segment, std::cref(divisors), std::ref(results));
            }
        }
        for(auto& th : thread_list) th.join();

        auto end = std::chrono::steady_clock::now();
        std::string end_stamp = stamp();
        auto time_elapsed = end - start;
        std::chrono::duration<double, std::milli> ms = end - start;

        // the finale
        std::vector<int> all;
        size_t total = 0;
        for (auto& r : results) total += r.size();
        all.reserve(total);
        for (auto& r : results) {
            all.insert(all.end(), r.begin(), r.end());
        }
        std::sort(all.begin(), all.end());
        for (const auto& r : all) std::cout << r << " ";
        std::cout << std::endl;
        std::cout << "End: " << end_stamp << "\n";
        std::cout << "Elapsed: " << ms.count() << " ms\n";
        std::cout << std::endl;
    }
    return 0;
}