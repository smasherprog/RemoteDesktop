#ifndef DESKTOP_CAPTURE_CONTAINER123_H
#define DESKTOP_CAPTURE_CONTAINER123_H
#include <vector>
#include "Image.h"
#include <memory>

namespace RemoteDesktop{
	std::vector<std::shared_ptr<Image>> CaptureDesktops();
}

#endif