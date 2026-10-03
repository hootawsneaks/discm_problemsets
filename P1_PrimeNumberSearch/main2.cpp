#include <iostream>
#include <fstream>
#include <string>
#include <thread>

/*
 first is dividing threads by the max range (100 range divided by 4 threads) and also testing divisibility on that same thread
*/
std::vector<int> primecheck_job_scheme1(int id, int range_begin, int range_end, int range_total){
    std::vector<int> prime_segment(range_end - range_begin);
    for (int i = 2; i * i <= range_total; i++){

    }
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
}