#ifndef VECTORPOOL_123_H
#define VECTORPOOL_123_H
#include <vector>
#include <mutex>

namespace RemoteDesktop{
	class VectorPool{
		std::vector<std::vector<char>> Pool;
		std::mutex PoolLock;
	public:
		std::vector<char> get_Buffer(int size);
		void release_Buffer(std::vector<char>& v);
	};

}

#endif