#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>

#include <chrono>
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

#define THREAD_COUNT 8

int modmul(int a, int b, int mod) {
	return (int)((int64_t) a * b % mod);
}

int modpow(int base, int exp, int mod) {
	int ans = 1;
	while (exp) {
		if (exp % 2 == 1)
			ans = modmul(ans, base, mod);
		base = modmul(base, base, mod);
		exp /= 2;
	}
	return ans;
}

// implementation of the [Miller-Rabin primality test](https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test)
bool isPrime(int n) {
	if (n < 2 || n % 6 % 4 != 1)
		return (n | 1) == 3;

	// bases taken from https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test#Testing_against_small_sets_of_bases
	int bases[] = {2, 3, 5, 7};

	int s = __builtin_ctz(n - 1);
	int d = (n - 1) >> s;

	for (int a : bases) {
		if (a >= n)
			break;

		int x = modpow(a, d, n);

		for (int i = 0; i < s; ++i) {
			int y = modmul(x, x, n);
			if (y == 1 && x != 1 && x != n - 1)
				return false; // composite
			x = y;
			if (i == s - 1 && y != 1)
				return false; // composite
		}
	}

	return true; // prime
}

// counts and sums all the primes in the range [lo, hi)
void countPrimes(
	std::atomic<int> &primeCount, std::atomic<uint64_t> &primeSum,
	std::atomic<uint64_t> &top10Min, std::vector<uint64_t> &top10Primes, std::mutex &top10Mutex,
	int lo, int hi
) {
	for (int i = lo; i < hi; ++i) {
		if (isPrime(i)) {
			primeCount += 1;
			primeSum += i;

			if (i > top10Min.load()) {
				top10Mutex.lock();

				top10Primes.push_back(i);
				sort(top10Primes.begin(), top10Primes.end());
				top10Primes.erase(top10Primes.begin());

				top10Min = top10Primes[0];

				top10Mutex.unlock();
			}
		}
	}
}

// spawns THREAD_COUNT threads, and splits the work evenly among them in order to count up the total amount
// and sum of primes in the range [lo, hi)
std::tuple<int, uint64_t, std::vector<uint64_t>> parallelCountPrimes(int lo, int hi) {

	// initialize atomic variables
	std::atomic<int> primeCount = 0;
	std::atomic<uint64_t> primeSum = 0;

	std::atomic<uint64_t> top10Min = 0;
	std::vector<uint64_t> top10Primes(10, 0);
	std::mutex top10Mutex;

	// create threads
	std::vector<std::thread> threads;
	for (int i = 0; i < THREAD_COUNT; ++i) {
		// figure out what range to dedicate to each thread
		int threadLo = lo + (hi - lo) * i / THREAD_COUNT;
		int threadHi = lo + (hi - lo) * (i + 1) / THREAD_COUNT;

		// start the thread
		threads.emplace_back(countPrimes,
			std::ref(primeCount), std::ref(primeSum),
			std::ref(top10Min), std::ref(top10Primes), std::ref(top10Mutex),
			threadLo, threadHi
		);
	}

	// wait for all threads to finish
	for (std::thread &thr : threads)
		thr.join();

	// return the final result
	return std::make_tuple((int) primeCount, (uint64_t) primeSum, top10Primes);

}

int main() {
	auto start = high_resolution_clock::now();

	// parallelCountPrimes takes in an inclusive-exclusive range, hence the + 1
	auto [primeCount, primeSum, top10Primes] = parallelCountPrimes(1, 100'000'000 + 1);

	auto end = high_resolution_clock::now();

	std::cout
		<< duration_cast<milliseconds>(end - start).count() << "ms"
		<< " " << primeCount
		<< " " << primeSum << std::endl;

	for (uint64_t prime : top10Primes)
		std::cout << prime << std::endl;

	// side note:
	// the following commented-out single-threaded prime sieve code runs in 0.312s on my machine with g++ -O2,
	// easily beating every other multi-threaded solution I was able to come up with:
	/*
	std::vector<bool> sieve(100'000'001, true);
	sieve[0] = sieve[1] = false;
	for (int i = 2; i * i < sieve.size(); ++i) {
		if (!sieve[i])
			continue;
		for (int j = i * i; j < sieve.size(); j += i)
			sieve[j] = false;
	}

	int count = 0;
	for (int i = 1; i <= 100'000'000; ++i) {
		if (sieve[i])
			count += 1;
	}
	std::cout << count << std::endl;
	*/

	return 0;
}
