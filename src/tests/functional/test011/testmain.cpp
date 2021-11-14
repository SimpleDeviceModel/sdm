// Allow assertions in Release mode
#ifdef NDEBUG
	#undef NDEBUG
#endif

#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <cassert>

std::timed_mutex m;

void threadProc() {
	m.lock();
	std::cout<<"Locked mutex in the worker thread"<<std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	m.unlock();
}

int main(int argc,char *argv[]) {
	std::thread t(threadProc);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout<<"Trying to lock mutex in the main thread"<<std::endl;
	auto b=m.try_lock_for(std::chrono::milliseconds(200));
	assert(b);
	t.join();
	m.unlock();
	
	return 0;
}
