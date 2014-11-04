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
	class Point{
	public:
		Point(int t, int l) : top(t), left(l) {}
		Point(){}
		int top = 0;
		int left = 0;

	};
#pragma pack(pop)
	inline bool operator ==(const Point& l, const Point& r){
		return l.left == r.left && l.top == r.top;
	}
	inline bool operator !=(const Point& l, const Point& r){
		return !(l == r);
	}
};

#endif