#pragma once

#define _WINSOCKAPI_
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif
void* MsfScreenShot_create();
void MsfScreenShot_InitRawDesktop(void* msfss);
void MsfScreenShot_EncodeData(void* msfss, BYTE **out, DWORD *size, GUID encode_format);
UINT MsfScreenShot_GetWidth(void* msfss);
UINT MsfScreenShot_GetHeight(void* msfss);
DWORD MsfScreenShot_SendScreenShot(void* msfss, char * cpNamedPipe, BYTE * pJpegBuffer, DWORD dwJpegSize);
void MsfScreenShot_delete(void* msfss);
#ifdef __cplusplus
}
#endif