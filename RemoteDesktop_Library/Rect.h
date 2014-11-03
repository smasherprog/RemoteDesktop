#ifndef RECTANGLE_H
#define RECTANGLE_H

namespace RemoteDesktop{
#pragma pack(push, 1)
	class Rect{
	public:
		Rect(int t, int l, int w, int h) : top(t), left(l), width(w), height(h) {}
		Rect(){}
		int top = 0;
		int left = 0;
		int width = 0;
		int height = 0;
	};
#pragma pack(pop)
};

#endif