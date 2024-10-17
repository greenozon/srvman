#pragma once
#include "../bzs_string.h"

namespace BazisLib
{
	namespace KEXT
	{
		DynamicStringA UTF16ToUTF8(const wchar16_t *pString, size_t length = -1);
		_DynamicStringT<wchar16_t> UTF8ToUTF16(const char *pString, size_t length = -1);
	}
}