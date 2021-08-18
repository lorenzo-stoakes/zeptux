#include "test_user.h"

#include "spinlock.h"
#undef static_assert // zeptux breaks static_assert in c++ :)

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define NUM_READER_THREADS (100)
#define NUM_WRITER_THREADS (100)

#define DURATION_SECS (5)

#define BUF_SIZE (1000)

// Shared state protected by the spinlock.
struct {
	char buf[BUF_SIZE] = {0};
	spinlock_t lock = empty_spinlock();
} shared;

// We maintain state protected by a C++ mutex in order to count failures
// correctly.
struct {
	std::atomic<int> reads = 0;
	std::atomic<int> writes = 0;
	int faults = 0;
	char curr_chr = 'a';
	bool stop = false;
	std::mutex mutex;
} test_state;

namespace {
bool should_stop()
{
	std::lock_guard(test_state.mutex);
	return test_state.stop;
}

void set_stop()
{
	std::lock_guard(test_state.mutex);
	test_state.stop = true;
}

char cycle_curr_chr()
{
	std::lock_guard(test_state.mutex);

	char &chr = test_state.curr_chr;
	chr = chr == 'z' ? 'a' : chr + 1;

	return chr;
}

void reader()
{
	spinlock_acquire(&shared.lock);

	char first = shared.buf[0];
	for (int i = 1; i < BUF_SIZE; i++) {
		if (shared.buf[i] != first) {
			std::lock_guard(test_state.mutex);
			test_state.faults++;
			break;
		}
	}

	spinlock_release(&shared.lock);
}

void writer()
{
	char chr = cycle_curr_chr();

	spinlock_acquire(&shared.lock);

	// memset is messed with in zeptux so do this the old fashioned way.
	for (int i = 0; i < BUF_SIZE; i++) {
		shared.buf[i] = chr;
	}

	spinlock_release(&shared.lock);
}
} // namespace

std::string test_spinlock()
{
	std::cout << "test_spinlock: starting run across " << NUM_READER_THREADS
		  << " readers and " << NUM_WRITER_THREADS << " writers for "
		  << DURATION_SECS << "s (buffer size " << BUF_SIZE << " bytes)"
		  << std::endl;

	std::vector<std::thread> writer_threads;
	for (int i = 0; i < NUM_WRITER_THREADS; i++) {
		writer_threads.emplace_back([&]() {
			while (true) {
				if (should_stop())
					break;
				writer();
				test_state.writes++;
			}
		});
	}

	std::vector<std::thread> reader_threads;
	for (int i = 0; i < NUM_READER_THREADS; i++) {
		reader_threads.emplace_back([&]() {
			while (true) {
				if (should_stop())
					break;

				reader();
				test_state.reads++;
			}
		});
	}

	std::this_thread::sleep_for(std::chrono::seconds(DURATION_SECS));

	set_stop();
	for (auto &r : reader_threads) {
		r.join();
	}
	for (auto &w : writer_threads) {
		w.join();
	}

	std::cout << "test_spinlock: " << test_state.reads << " reads, "
		  << test_state.writes << " writes" << std::endl;
	assert(test_state.faults == 0,
	       std::to_string(test_state.faults) + " faults!");

	return "";
}
