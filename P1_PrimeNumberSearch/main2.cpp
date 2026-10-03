#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

void primecheck_job_scheme1(int id, int range_begin, int range_end, std::vector<int> divisors, std::vector<std::vector<int>> &results){
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
    while (file >> key){
        if (key == "range") {
            file >> range;
        }
        else if (key == "threads") {
            file >> threads;
        }
    }
    // add input testing here

    thread_list.reserve(threads);
    std::vector<std::vector<int>> results(threads);
    // precompute division gonna be used
    std::vector<int> divisors;
    for(int i = 2; i * i <= range; i++){
        divisors.push_back(i);
    }

    // job to split threads
    int segment = (range - 1) / threads;
    int id = 1;
    if (threads <= 0){
        std::cout << "Threads cannot be less than 1." << std::endl;
    }
    else if (threads >= 1){
        for(int i = 2; i <= range; i += segment + 1){
            if(i + segment >= range){
                //std::cout << i << " " << range << std::endl;
                //primecheck_job_scheme1(id++, i, range, divisors, results);
                thread_list.emplace_back(primecheck_job_scheme1, id++, i, range, divisors, results);
            }
            else{
                //std::cout << i << " " << i + segment << std::endl;
                //primecheck_job_scheme1(id++, i, i + segment, divisors, results);
                thread_list.emplace_back(primecheck_job_scheme1, id++, i, i + segment, divisors, results);
            }
        }
        for(auto& th : thread_list) th.join();

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
    }
    else {

    }
    return 0;
}