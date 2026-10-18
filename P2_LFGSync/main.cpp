#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

std::mutex mtx;

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps, int t1, int t2){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(t1, t2);
    bool isEmpty = true;
    // main loop aka critical-ish section..
    while(isEmpty){
        {
            std::unique_lock<std::mutex> lock(mtx);
            if(num_dps == 3 && num_tank == 1 && num_healer == 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;
                isEmpty = false;
            }
            else{
                break;
            }
        }
        // wait until they finish their dungeon run.
        std::this_thread::sleep_for(std::chrono::seconds(distrib(gen)));
        isEmpty = true;
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

    // assume everything initialized beyond here


    

    return 0;
}