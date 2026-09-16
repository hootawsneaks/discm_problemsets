/*
- Find prime numbers from 1 -> y given x threads
- separate config file for setting up x and y
- printing variations include printing immediately or waiting until all threads have finished
- also different task division schemes
- first is dividing threads by the max range (100 range divided by 4 threads) and also testing divisibility on that same thread
-  second division schemes is that search is linear but threads are the one doing the divisibility testing
- theres the secret third variant where its a mix of the first and second task division scheme (NOT INCLUDED)
- make sure to observe the performance characteristics
- nodejs and python IS NOT ALLOWED for this problem set, as well as problem set 2
- INDIVIDUAL
*/