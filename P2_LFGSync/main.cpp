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
#include <ranges>

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

            if(num_dps >= 3 && num_tank >= 1 && num_healer >= 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;
                ++num_working;
                ++num_served;
                working_list[id] = 1;
                ready = false;
            }
            else{
                isWorking = false;
                done = true;
                ready = false;
                cv.notify_all();
                continue;
            }
        }
        // wait until they finish their dungeon run.
        cv2.notify_all();
        std::this_thread::sleep_for(std::chrono::seconds(distrib(gen)));
        {
            std::unique_lock<std::mutex> lock(mtx);
            --num_working;
        }
    }
}

void signaling_worker(int instances, std::vector<uint8_t>& working_list){
   std::vector<int> patience_list(instances, 0);
   // data race...
   while (true){
    /* this lock is done so that it can make sure no one is touching the thing before assigning someone a task
       since working_list is touched by threads, even if the reads dont clash, loops are probably dangerous.
       the lock should be maintained during the entirety of the choosing of a worker
    */
        {
            std::unique_lock<std::mutex> lock(mtx);
            if(done){
                break;
            }
            if (num_working == instances){
                {
                    std::unique_lock<std::mutex> lock2(mtx2);
                    cv2.wait( lock2, [&] { return ( num_working < instances ); });
                }
            }

            target_id = static_cast<int>(std::ranges::max_element(patience_list) - patience_list.begin());
            if (working_list[target_id] == 1){
                target_id = -1;
            }
            else{
                patience_list[target_id] = 0;
            }
            const int n = static_cast<int>(patience_list.size());
            for (int i = 0; i < n; ++i){
                if ( i != target_id && !working_list[i] ) patience_list[i]++;
            }
            std::cout << "== LIST OF INSTANCES: ==\n";
            for (int i = 0; i < instances; ++i){
                std::cout << "Instance " << i << " (" << (working_list[i] ? "Active" : "Empty") << ")\n";
                std::cout << std::endl;
            }
            ready = true;
        }
        cv.notify_all();
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

    // todo: input checks

    // "bootstrapping"
    std::vector<uint8_t> working_list(instances, 0);
    std::vector<std::thread> thread_list;
    thread_list.reserve(instances + 1);

     for(int i = 0; i < instances; ++i){
        thread_list.emplace_back(instance_worker, i, std::ref(num_tank), std::ref(num_healer), std::ref(num_dps), t1, t2, std::ref(working_list));
    }
    thread_list.emplace_back(signaling_worker, instances, std::ref(working_list));
    for(auto& t : thread_list){
        if (t.joinable()) t.join();
    }
    return 0;
}