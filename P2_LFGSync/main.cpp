#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>

std::mutex mtx;

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps){
    bool isEmpty = true;
    while(isEmpty){
        {
            std::lock_guard<std::mutex> lock(mtx);
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


    

    return 0;
}