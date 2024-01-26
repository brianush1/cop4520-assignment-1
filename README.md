# Prime counting assignment

To compile the program, ensure you have `gcc` installed, and then run the following command in the directory of the repo:

```
g++ -O2 -o assignment assignment.cpp
```

In order to run the program, use the command:

```
./assignment
```

## Approach

My approach was to take the range [1, 10^8], and split it up into 8 equal-sized ranges, which could be processed independently by each of the 8 threads I spawned. Then, each thread would iterate through the numbers in the range and check if the number is prime; if it is, it would increment the prime count, add to the prime sum, and update the "top 10 primes" list. To ensure that there were no race conditions, I used atomic additions for the prime count and sum, and I used a mutex in order to update the "top 10 primes" list when necessary.

Since the ranges that the threads are responsible for are disjoint, there will never be a prime that is counted twice; since the ranges are adjacent, there will never be a prime that should be counted that isn't.

In order to check if a number is prime, I opted for the Miller-Rabin primality test, since it's known to be deterministic for numbers in a certain range (up to around 3 billion), and it's much quicker than any other non-sieve primality test.

## Experimental evaluation

I ran the program 8 times on my machine. Here are the times that each run took:

| Run | Time       |
|-----|------------|
|   1 | 1452ms     |
|   2 | 961ms      |
|   3 | 1448ms     |
|   4 | 969ms      |
|   5 | 961ms      |
|   6 | 940ms      |
|   7 | 937ms      |
|   8 | 960ms      |
| Avg | 1078.5ms   |

The output for the first run was as follows (and every other run produced identical results, other than the time):

```
1452ms 5761455 279209790387276
99999787
99999821
99999827
99999839
99999847
99999931
99999941
99999959
99999971
99999989
```
