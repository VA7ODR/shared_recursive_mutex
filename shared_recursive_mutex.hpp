#pragma once
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>
#include <unordered_map>

using namespace std::literals;

class SharedRecursiveMutex {
	public:
		void lock_shared(std::chrono::milliseconds timeout = 300ms)
		{
			auto start = std::chrono::steady_clock::now();
			std::unique_lock<std::mutex> lk(mutex_);
			auto tid = std::this_thread::get_id();

			// std::cout << ">>> A \"" << exclusive_owner << "\", \"" << tid << "\", E: " << exclusive_count_ << ", S: " << shared_owners.size() << std::endl;

			auto isGood = [this, &tid]() {
				return exclusive_count_ == 0 || exclusive_owner == tid;
			};
			if (!shared_cv_.wait_for(lk, timeout, isGood)) {
				std::cerr << "Shared lock timed out. (" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms) Continuing to wait." << std::endl;
				shared_cv_.wait(lk, isGood);
				std::cerr << "Shared lock released after " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms." << std::endl;
			}

			// std::cout << ">>> B \"" << exclusive_owner << "\", \"" << tid << "\", E: " << exclusive_count_ << ", S: " << shared_owners.size() << std::endl;

			assert (exclusive_count_ == 0 || exclusive_owner == tid);

			++shared_owners[tid];
			shared_cv_.notify_all();
		}

		void unlock_shared()
		{
			std::unique_lock<std::mutex> lk(mutex_);
			auto tid = std::this_thread::get_id();
			auto & tid_data = shared_owners[tid];
			--tid_data;

			if (tid_data == 0) {
				shared_owners.erase(tid);
				if (shared_owners.empty()) {
					exclusive_cv_.notify_one();
				}
			}
		}

		void lock(std::chrono::milliseconds timeout = 300ms)
		{
			auto start = std::chrono::steady_clock::now();
			std::unique_lock<std::mutex> lk(mutex_);
			auto tid = std::this_thread::get_id();

			auto isGood = [this, &tid] {
				if (shared_owners.size() > 1 || (shared_owners.size() == 1 && shared_owners.begin()->first != tid)) {
					return false;
				}
				return (exclusive_count_ == 0) || (tid == exclusive_owner);
			};

			// std::cerr << ">>> A \"" << exclusive_owner << "\", \"" << tid << "\", E: " << exclusive_count_ << ", S: " << shared_owners.size() << std::endl;

			if (!exclusive_cv_.wait_for(lk, timeout, isGood)) {
				std::cerr << "Exclusive lock timed out. (" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms) Continuing to wait." << std::endl;
				exclusive_cv_.wait(lk, isGood);
				std::cerr << "Exclusive lock released after " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms." << std::endl;
			}

			// std::cerr << ">>> B \"" << exclusive_owner << "\", \"" << tid << "\", E: " << exclusive_count_ << ", S: " << shared_owners.size() << std::endl;

			assert((exclusive_count_ == 0 || exclusive_owner == tid) && (shared_owners.empty() || (shared_owners.size() == 1 && shared_owners.begin()->first == tid)));

			exclusive_owner = tid;
			++exclusive_count_;
		}

		void unlock()
		{
			std::unique_lock<std::mutex> lk(mutex_);
			--exclusive_count_;

			if (exclusive_count_ == 0) {
				exclusive_owner = std::thread::id();
				exclusive_cv_.notify_one();
				shared_cv_.notify_one();
			}
		}

		std::pair<int, int> counts()
		{
			auto sum_shared = [this]() {
				return  std::accumulate(shared_owners.begin(), shared_owners.end(), 0,
										[](int currentSum, const std::pair<std::thread::id, int>& keyValue) {
											return currentSum + keyValue.second;
										});
			};
			std::unique_lock<std::mutex> lk(mutex_);
			return { exclusive_count_, sum_shared() };
		}
	private:
		std::mutex mutex_;
		std::condition_variable exclusive_cv_;
		std::condition_variable shared_cv_;
		std::unordered_map<std::thread::id, int> shared_owners;
		int exclusive_count_ { 0 };
		std::thread::id exclusive_owner {};
};

class RecursiveSharedLock
{
	public:
		RecursiveSharedLock(SharedRecursiveMutex & mtxIn, std::chrono::milliseconds timeout = 300ms) : mtx(mtxIn)
		{
			mtx.lock_shared(timeout);
		}

		~RecursiveSharedLock()
		{
			mtx.unlock_shared();
		}

		std::pair<int, int> counts() { return mtx.counts(); }

	private:
		SharedRecursiveMutex & mtx;
};

class RecursiveExclusiveLock
{
	public:
		RecursiveExclusiveLock(SharedRecursiveMutex & mtxIn, std::chrono::milliseconds timeout = 300ms) : mtx(mtxIn)
		{
			mtx.lock(timeout);
		}

		~RecursiveExclusiveLock()
		{
			mtx.unlock();
		}

		std::pair<int, int> counts() { return mtx.counts(); }

	private:
		SharedRecursiveMutex & mtx;
};


