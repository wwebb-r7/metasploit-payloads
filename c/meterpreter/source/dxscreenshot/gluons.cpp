#pragma once

#include "DXGIManager.h"
#include "MsfScreenShot.h"
#include "gluons.h"

extern "C" void* MsfScreenShot_create()
{
	return new MsfScreenShot;
}

extern "C" void MsfScreenShot_InitRawDesktop(void* msfss)
{
	static_cast<MsfScreenShot*>(msfss)->InitializeRawDesktopBitmap();
}

extern "C" void MsfScreenShot_EncodeData(void* msfss, BYTE **out, DWORD *size, GUID encode_format)
{
	static_cast<MsfScreenShot*>(msfss)->EncodeData(out, size, encode_format);
}

extern "C" UINT MsfScreenShot_GetWidth(void* msfss)
{
	return static_cast<MsfScreenShot*>(msfss)->GetWidth();
}

extern "C" UINT MsfScreenShot_GetHeight(void* msfss)
{
	return static_cast<MsfScreenShot*>(msfss)->GetHeight();
}

extern "C" DWORD MsfScreenShot_SendScreenShot(void* msfss, char * cpNamedPipe, BYTE * pJpegBuffer, DWORD dwJpegSize)
{
	DWORD ret = static_cast<MsfScreenShot*>(msfss)->SendScreenShot(cpNamedPipe, pJpegBuffer, dwJpegSize);
	return ret;
}

extern "C" void MsfScreenShot_delete(void* msfss)
{
	delete static_cast<MsfScreenShot*>(msfss);
}