// an attempt at organization

// essentials
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <syncstream>

// string manip
#include <string>
#include <format>

// rng & time
#include <chrono>
#include <random>

// convenience
#include <algorithm>
#include <vector>

std::mutex mtx, mtx2;
std::condition_variable cv, cv2;
bool ready = false;
bool done = false;
int num_served = 0;
int num_working = 0;
int target_id = -1;

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps, int t1, int t2, std::vector<uint8_t>& working_list){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(t1, t2);
    bool isWorking = true;
    while(isWorking){
        {
            std::unique_lock<std::mutex> lock(mtx);

            cv.wait(lock, [&] {
                return ready && (target_id == id);
            });

            ++num_working;

            if(num_dps >= 3 && num_tank >= 1 && num_healer >= 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;

                --num_working;
                ready = false;
            }
            else{
                isWorking = false;
                done = true;
                --num_working;
                ready = false;
                break;
            }
        }
        // wait until they finish their dungeon run.
        cv2.notify_all();
        working_list[id] = 1;
        std::this_thread::sleep_for(std::chrono::seconds(distrib(gen)));
    }
}

void signaling_worker(int instances, std::vector<uint8_t>& working_list){
   std::vector<int> patience_list(instances);
   // init
   for (int &i : patience_list){
        i = 0;
   }
   
   // mootex
   while (!done){
        {
            std::unique_lock<std::mutex> lock(mtx);
            target_id = std::distance(patience_list.begin(), std::min_element(patience_list.begin(), patience_list.end()));
            patience_list[target_id] = 0;
            for (int i = 0; i <= (int)patience_list.size(); ++i){
                if ( i != target_id && !working_list[i] ) patience_list[i]++;
            }
            ready = true;
        }
        cv.notify_all();
        for (int i = 0; i < instances; ++i){
            std::cout << "Instance " << i << " (" << (working_list[i] ? "Active" : "Empty") << ")\n";
            std::cout << std::endl;
        }
        if (num_working == instances){
            {
                std::unique_lock<std::mutex> lock(mtx2);
                cv2.wait( lock, [&] { return ( num_working < instances ); });
            }
        }
    }
}

int main(){
    std::ifstream file("config.txt");
    if (!file.is_open()){
        std::cerr << "config txt malformed or missing\n";
        return 1;
    }
    // init
    std::string key;
    long buffer = 0;
    // input
    int instances;
    int num_tank;
    int num_healer;
    int num_dps;
    int t1;
    int t2;


    while (file >> key){
        // parse 123abc when?
        if (!(file >> buffer)){
            std::cerr << "invalid input detected. please check the config file for proper inputs\n";
            return 1;
        }
        if (key == "instances"){
            instances = static_cast<int>(buffer);
        }
        else if (key == "num_tank"){
            num_tank = static_cast<int>(buffer);
        }
        else if (key == "num_healer"){
            num_healer = static_cast<int>(buffer);
        }
        else if (key == "num_dps"){
            num_dps = static_cast<int>(buffer);
        }
        else if (key == "t1"){
            t1 = static_cast<int>(buffer);
        }
        else if (key == "t2"){
            t2 = static_cast<int>(buffer);
        }
    }

    // input checks

    // "bootstrapping"
    std::vector<uint8_t> working_list(instances, 0);
    std::vector<std::thread> thread_list(instances);

     for(int i = 0; i < instances; ++i){
        thread_list.emplace_back(instance_worker, i, std::ref(num_tank), std::ref(num_healer), std::ref(num_dps), t1, t2, std::ref(working_list));
    }
    thread_list.emplace_back(signaling_worker, instances, std::ref(working_list));

    

    return 0;
}