#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

/*
 first is dividing threads by the max range (100 range divided by 4 threads) and also testing divisibility on that same thread
*/

std::vector<int> primecheck_job_scheme1(int id, int range_begin, int range_end, std::vector<int> divisors){
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
    return result;
} 


int main(){
    std::ifstream file("config.txt");
    std::string key;
    int threads = 1, range = 0;
    while (file >> key){
        if (key == "range") {
            file >> range;
        }
        else if (key == "threads") {
            file >> threads;
        }
    }
    // precompute division gonna be used
    std::vector<int> divisors;
    for(int i = 2; i * i <= range; i++){
        divisors.push_back(i);
    }

    // job to split threads
    if (threads <= 0){
        std::cout << "Threads cannot be less than 1." << std::endl;
    }
    else if (threads >= 1){
        
    }
    else {
        primecheck_job_scheme1(1, 2, 100, divisors);
    }
    std::cout << std::endl;
}