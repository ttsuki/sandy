#ifndef PCH_H
#define PCH_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <combaseapi.h>
#include <mmsystem.h>
#include <dxgi1_6.h>
#include <d3d11_4.h>

#include <xtw/xtw.h> // https://github.com/ttsuki/xtw/

#endif //PCH_H
