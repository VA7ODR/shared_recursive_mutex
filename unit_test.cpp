#define BOOST_TEST_MODULE SharedRecursiveMutexTests
#include <boost/test/included/unit_test.hpp>
#include "shared_recursive_mutex.hpp"

BOOST_AUTO_TEST_SUITE(SharedRecursiveMutexTests)

BOOST_AUTO_TEST_CASE(testSharedLock) {
	SharedRecursiveMutex mutex;

	{
		RecursiveSharedLock lock(mutex);
		auto counts = lock.counts();

		BOOST_TEST_REQUIRE(counts.first == 0); // Exclusive count
		BOOST_TEST_REQUIRE(counts.second == 1); // Shared count

		{
			RecursiveSharedLock lock2(mutex);
			counts = lock2.counts();

			BOOST_TEST_REQUIRE(counts.first == 0);
			BOOST_TEST_REQUIRE(counts.second == 2);
		}

		counts = lock.counts();
		BOOST_TEST_REQUIRE(counts.first == 0);
		BOOST_TEST_REQUIRE(counts.second == 1);
	}

	auto counts = mutex.counts();
	BOOST_TEST_REQUIRE(counts.first == 0);
	BOOST_TEST_REQUIRE(counts.second == 0);
}

BOOST_AUTO_TEST_CASE(testExclusiveLock) {
	SharedRecursiveMutex mutex;

	{
		RecursiveExclusiveLock lock(mutex);
		auto counts = lock.counts();

		BOOST_TEST_REQUIRE(counts.first == 1); // Exclusive count
		BOOST_TEST_REQUIRE(counts.second == 0); // Shared count

		{
			RecursiveExclusiveLock lock2(mutex);
			counts = lock2.counts();

			BOOST_TEST_REQUIRE(counts.first == 2);
			BOOST_TEST_REQUIRE(counts.second == 0);
		}

		counts = lock.counts();
		BOOST_TEST_REQUIRE(counts.first == 1);
		BOOST_TEST_REQUIRE(counts.second == 0);
	}

	auto counts = mutex.counts();
	BOOST_TEST_REQUIRE(counts.first == 0);
	BOOST_TEST_REQUIRE(counts.second == 0);
}

BOOST_AUTO_TEST_SUITE_END()
