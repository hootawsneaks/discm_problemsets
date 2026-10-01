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

int main() {
  
  // init 
  int range = 0, threads = 1;
  std::ifstream file("config.txt");
  std::string key;
  while (file >> key) {
    if (key == "range")
      file >> range;
    else if (key == "threads")
      file >> threads;
  }

  // setup of bool vector
  std::vector<bool> isComposite(range + 1, false);
  isComposite[0] = true;
  isComposite[1] = true;

  // sieve
  for (int i = 2; i * i <= range; i++) {
    for (int j = i * i; j <= range; j += i) {
      isComposite[j] = true;
    }
  }

  // okay so the end result here SHOULD be sieve completed.
  for (int i = 2; i <= range; i++) {
    if (!isComposite[i]) {
      std::cout << i << " ";
    }
  }
}
