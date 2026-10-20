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

std::mutex mtx;
std::condition_variable cv, cv2;
bool ready = false;
bool done = false;
int num_served = 0;
int num_working = 0;
int target_id = -2;
int signal_iter = 0;

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps, int t1, int t2, std::vector<uint8_t>& working_list){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(t1, t2);
    bool isWorking = true;
    while(isWorking){
        {
            std::unique_lock<std::mutex> lock(mtx);

            if (done) {
                ready = false;
                cv2.notify_all();
                break;
            }

            // -1 meaning emergency
            cv.wait(lock, [&] {
                return (ready && (target_id == id)) || ( target_id == -1 );
            });

            ready = false;

            if(num_dps >= 3 && num_tank >= 1 && num_healer >= 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;
                ++num_working;
                std::osyncstream(std::cout) << "[WORKER] Number of instances working: " << num_working << "\n";
                ++num_served;
                working_list[id] = 1;
                std::osyncstream(std::cout) << "INSTANCE [" << id << "] FILLED\n";
                
            }
            else{
                isWorking = false;
                cv2.notify_all();
                continue;
            }
        }
        // notify when a worker lets go of lock
        cv2.notify_all();
        // signaler will have the lock if all instances are 'busy'
         std::osyncstream(std::cout) << "INSTANCE [" << id << "] RUNNING/SLEEPING\n";
        std::this_thread::sleep_for(std::chrono::seconds(distrib(gen)));
        {
            std::unique_lock<std::mutex> lock(mtx);
            working_list[id] = 0;
            std::osyncstream(std::cout) << "INSTANCE [" << id << "] RUN DONE!\n";
            --num_working;
            // notify when a worker finished a run
            cv2.notify_all();
        }
    }
}

void signaling_worker(int instances, std::vector<uint8_t>& working_list, int& num_tank, int& num_healer, int& num_dps){
   std::vector<int> patience_list(instances, 0);
   while (true){
        {
            std::unique_lock<std::mutex> lock(mtx);
            ++signal_iter;

            if (done) break;

            std::osyncstream(std::cout) << "[SIGNALER] Number of instances working: " << num_working << "\n";
            if (num_working == instances){
                {
                    std::osyncstream(std::cout) << "SIGNALER SLEEPING... " << num_working << "\n";
                    cv2.wait( lock, [&] { return ( num_working < instances ); });
                    std::osyncstream(std::cout) << "SIGNALER AWAKENING!!! " << num_working << "\n";
                }
            }

            if(!(num_dps >= 3 && num_tank >= 1 && num_healer >= 1)){
                target_id = -1;
                done = true;
            }
            else{
                auto ids = std::views::iota(0, instances);
                target_id = *std::ranges::max_element(ids, {}, [&](int i) {
                    return working_list[i] == 1 ? -1 : patience_list[i];
                });
                std::osyncstream(std::cout) << "SIGNALER CHOOSES INSTANCE " << target_id << "\n";
                patience_list[target_id] = 0;

                const int n = static_cast<int>(patience_list.size());
                for (int i = 0; i < n; ++i){
                    if ( i != target_id && !working_list[i] ) {
                        patience_list[i]++;
                        std::osyncstream(std::cout) << "PATIENCE++: " << i << ", total patience: " << patience_list[i] << "\n";
                    }
                }
            }

            std::osyncstream(std::cout)  << "== LIST OF INSTANCES: ==\n";
            for (int i = 0; i < instances; ++i){
                std::osyncstream(std::cout)  << "Instance " << i << " (" << (working_list[i] ? "Active" : "Empty") << ")\n";
                std::osyncstream(std::cout)  << std::endl;
            }
            ready = true;
        }
        cv.notify_all();
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv2.wait( lock, [&] { return ( !ready ); });
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

    // todo: input checks

    // "bootstrapping"
    std::vector<uint8_t> working_list(instances, 0);
    std::vector<std::thread> thread_list;
    thread_list.reserve(instances + 1);

     for(int i = 0; i < instances; ++i){
        thread_list.emplace_back(instance_worker, i, std::ref(num_tank), std::ref(num_healer), std::ref(num_dps), t1, t2, std::ref(working_list));
    }
    thread_list.emplace_back(signaling_worker, instances, std::ref(working_list), std::ref(num_tank), std::ref(num_healer), std::ref(num_dps));
    for(auto& t : thread_list){
        if (t.joinable()) t.join();
    }

    // output results
    std::cout << "num served: " << num_served << "\n";
    std::cout << "tank plrs remaining " << num_tank << "\n";
    std::cout << "healers plrs remaining " << num_healer << "\n";
    std::cout << "dps plrs remaining " << num_dps << std::endl;
    std::cout << "total signaler iterations " << signal_iter << std::endl;

    return 0;
}