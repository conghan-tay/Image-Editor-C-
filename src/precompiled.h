#ifndef _PRECOMPILED_H
#define _PRECOMPILED_H

#include <cstddef>
#include <cstdint>
#include <cctype>
#include <crtdbg.h>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include "wx/wx.h"
#include "Window.h"
#include "Application.h"

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif /* NDEBUG */


#endif /* _PRECOMPILED_H */
