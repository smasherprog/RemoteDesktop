#ifndef CONCURRENT_QUEUE123_H
#define CONCURRENT_QUEUE123_H
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace RemoteDesktop{
	template <typename T>
	class Concurrent_Queue
	{
	public:
		Concurrent_Queue(){
			_Running = true;
		}
		~Concurrent_Queue(){
			ShutDown();
		}
		T pop()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (_Running && queue_.empty()) {
				cond_.wait(mlock);
			}
			if (!_Running) return T();//must return!
			auto item = queue_.back();
			queue_.pop_back();
			return item;
		}	
		void pop(T& item)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (_Running && queue_.empty()) cond_.wait(mlock);
			if (!_Running) return T();//must return!
			item = std::move(queue_.back());
			queue_.pop_back();
		}
		void push(const T& item)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			queue_.push_back(item);
			mlock.unlock();
			cond_.notify_one();
		}

		void emplace_back(T&& item)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			queue_.emplace_back(std::move(item));
			mlock.unlock();
			cond_.notify_one();
		}
		void ShutDown(){
			_Running = false;
			cond_.notify_all();
		}
	private:
		bool _Running = true;
		std::vector<T> queue_;
		std::mutex mutex_;
		std::condition_variable cond_;
	};
}
#endif