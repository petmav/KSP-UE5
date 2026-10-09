// Placeholder for the real kspcore static library. One exported function.
// Deliberately uses the STL so that the MSVC `#pragma detect_mismatch` records for RuntimeLibrary and
// _ITERATOR_DEBUG_LEVEL are emitted: if this lib were built /MDd or /MT, linking it into Unreal fails
// with LNK2038 exactly as a mis-built real core would.
#include "kspcore/kspcore_c.h"

#include <cstdio>
#include <string>

namespace
{
std::string MakeBuildInfo()
{
	char Buffer[256];
#if defined(_MSC_VER)
	const int Msc = _MSC_VER;
	const long Lang = static_cast<long>(_MSVC_LANG);
	const int Idl = _ITERATOR_DEBUG_LEVEL;
#else
	const int Msc = 0;
	const long Lang = static_cast<long>(__cplusplus);
	const int Idl = 0;
#endif
#if defined(_DLL) && defined(_DEBUG)
	const char* Crt = "MDd";
#elif defined(_DLL)
	const char* Crt = "MD";
#elif defined(_DEBUG)
	const char* Crt = "MTd";
#else
	const char* Crt = "MT";
#endif
	std::snprintf(Buffer, sizeof(Buffer), "kspcore-stub abi=%d msc=%d crt=%s idl=%d lang=%ld", KSPCORE_ABI_VERSION, Msc, Crt, Idl, Lang);
	return std::string(Buffer);
}
} // namespace

extern "C" const char* kspcore_build_info(void)
{
	static const std::string Info = MakeBuildInfo();
	return Info.c_str();
}
