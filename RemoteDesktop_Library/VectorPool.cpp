#include "stdafx.h"
#include "VectorPool.h"

std::vector<char> RemoteDesktop::VectorPool::get_Buffer(int size){
	std::lock_guard<std::mutex> lock(PoolLock);
	if (Pool.empty()) return std::vector<char>();
	auto p = Pool.back();
	Pool.pop_back();
	return p;
}
void RemoteDesktop::VectorPool::release_Buffer(std::vector<char>& v){
	std::lock_guard<std::mutex> lock(PoolLock);
	Pool.emplace_back(std::move(v));
}