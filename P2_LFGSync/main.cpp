#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <condition_variable>
#include <algorithm>

std::mutex mtx, mtx2;
std::condition_variable cv, cv2;

int target_id = -1;
bool ready = false;
int num_working = 0;
bool done = false;

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps, int t1, int t2){
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

            {
                std::unique_lock<std::mutex> lock(mtx2);
                ++num_working;
            }
            cv2.notify_all();

            if(num_dps >= 3 && num_tank >= 1 && num_healer >= 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;
                ready = false;
                {
                    std::unique_lock<std::mutex> lock(mtx2);
                    --num_working;
                }
                cv2.notify_all();
            }
            else{
                isWorking = false;
                ready = false;
                done = true;
                {
                    std::unique_lock<std::mutex> lock(mtx2);
                    --num_working;
                }
                cv2.notify_all();
                break;
            }
        }
        // wait until they finish their dungeon run.
        std::this_thread::sleep_for(std::chrono::seconds(distrib(gen)));
    }
}

void signaling_worker(int instances){
   std::vector<int> patience_list(instances);
   // init
   for (int &i : patience_list){
        i = 0;
   }
   
   // mootex
   while (!done){
    {
        std::unique_lock<std::mutex> lock(mtx2);
        cv2.wait(lock, [&] { return (num_working < instances); });
        std::unique_lock<std::mutex> lock(mtx);
        target_id = std::distance(patience_list.begin(), std::min_element(patience_list.begin(), patience_list.end()));
        patience_list[target_id] = 0;
        for (int i = 0; i <= (int)patience_list.size(); ++i){
            if (i != target_id) patience_list[i]++;
        }
        ready = true;
    }
    cv.notify_all();
    }
}

int main(){
    std::ifstream file("config.txt");
    if (!file.is_open()){
        std::cerr << "config txt malformed or missing" << std::endl;
        return 1;
    }
    // init
    std::string key;
    std::vector<std::thread> thread_list;
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
            std::cerr << "invalid input detected. please check the config file for proper inputs" << std::endl;
            return 1;
        }
        if (key == "instances"){
            instances = buffer;
        }
        else if (key == "num_tank"){
            num_tank = buffer;
        }
        else if (key == "num_healer"){
            num_healer = buffer;
        }
        else if (key == "num_dps"){
            num_dps = buffer;
        }
        else if (key == "t1"){
            t1 = buffer;
        }
        else if (key == "t2"){
            t2 = buffer;
        }
    }

    // input checks

    // "bootstrapping"
     for(int i = 0; i < instances; ++i){
        thread_list.emplace_back(instance_worker, i, std::ref(num_tank), std::ref(num_healer), std::ref(num_dps), t1, t2);
    }
    thread_list.emplace_back(signaling_worker, instances);

    

    return 0;
}