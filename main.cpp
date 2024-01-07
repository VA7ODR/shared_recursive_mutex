#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include "shared_recursive_mutex.hpp"

std::atomic<int> globalVariable = 0;
std::atomic_int threadCounter = 0;
SharedRecursiveMutex sharedMutex;

void writeThread(int id, int duration)
{
	threadCounter.fetch_add(1);
	{
		RecursiveExclusiveLock lock(sharedMutex);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration));

		// Exclusive lock acquired for writing
		globalVariable += 1;
		auto cts = lock.counts();
		std::cerr << "(E: " << cts.first << ", S: " << cts.second << ") Write Thread " << id << ": (" << duration << "ms) Incremented globalVariable to " << globalVariable << std::endl;
	}
	threadCounter.fetch_sub(1);
	// Exclusive lock automatically released when lock_guard goes out of scope
}

void readThread(int id, int duration)
{
	threadCounter.fetch_add(1);
	{
		RecursiveSharedLock lock(sharedMutex);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration));

		// Shared lock acquired for reading
		auto cts = lock.counts();
		std::cout << "(E: " << cts.first << ", S: " << cts.second << ") Read Thread " << id << ": (" << duration << "ms) Read globalVariable value: " << globalVariable << std::endl;
	}
	threadCounter.fetch_sub(1);
	// Shared lock automatically released when lock_guard goes out of scope
}

int main()
{
	constexpr int maxConcurrentThreads = 20;
	constexpr int maxThreadDuration = 1000; // milliseconds
	constexpr int minThreadDuration = 10; // milliseconds
	constexpr int totalRuntime = /*5 **/ 10 * 1000; // 5 minutes

	std::random_device rd;
	std::mt19937 gen(rd());
	std::gamma_distribution<> durationDistribution(2.0, 1.0 / (float)minThreadDuration);

	auto startTime = std::chrono::steady_clock::now();

	int threadID = 1;

	while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() < totalRuntime) {
		if (threadCounter <= maxConcurrentThreads) {
			int duration = durationDistribution(gen) * 500;
			std::cout << "     " << duration << std::endl;
			if (threadID % 2 == 0) {
				// Even threads: Write thread with exclusive lock
				std::thread(writeThread, threadID, duration).detach();
			} else {
				// Odd threads: Read thread with shared lock
				std::thread(readThread, threadID, duration).detach();
			}
			++threadID;
		}

		// Sleep for a short duration before starting the next iteration
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Give some time for threads to finish
	while (threadCounter) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return 0;
}
