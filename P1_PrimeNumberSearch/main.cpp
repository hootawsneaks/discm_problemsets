/*
- Find prime numbers from 1 -> y given x threads
- separate config file for setting up x and y
- printing variations include printing immediately or waiting until all threads
have finished
- also different task division schemes
- first is dividing threads by the max range (100 range divided by 4 threads)
and also testing divisibility on that same thread
-  second division schemes is that search is linear but threads are the one
doing the divisibility testing
- theres the secret third variant where its a mix of the first and second task
division scheme (NOT INCLUDED)
- make sure to observe the performance characteristics
- nodejs and python IS NOT ALLOWED for this problem set, as well as problem set
2
- INDIVIDUAL
*/

#include <fstream>
#include <iostream>
#include <string>
#include <thread>

// idea is have each thread have its own mini vector bool, then merge afterwards.
std::vector<bool> sieve(int id, int range_begin, int range_end){
  std::vector<bool> isComposite(range_end + 1, false);
  isComposite[0] = true;
  isComposite[1] = true;

  for (int i = range_begin; i * i <= range_end; i++) {
    for (int j = i * i; j <= range_end; j += i) {
      isComposite[j] = true;
    }
  }
  return isComposite;
}

int main() {

  // init range and threads
  int range = 0, threads = 1;
  std::ifstream file("config.txt");
  std::string key;
  while (file >> key) {
    if (key == "range")
      file >> range;
    else if (key == "threads")
      file >> threads;
  }

  if (threads > range){
    int excess = threads - range;
    // given the excess, it should all go to the first/last thread. for example: 10 threads 9 range. 2 numbers go to that.. wait.
  }
  else if (threads == range){

  }
  else{

  }

  for(int i = 1; i <= range; i += )

  //place holder
  std::vector<bool> isComposite(2, false);
  // okay so the end result here SHOULD be sieve completed.
  for (int i = 2; i <= range; i++) {
    if (!isComposite[i]) {
      std::cout << i << " ";
    }
  }
}
