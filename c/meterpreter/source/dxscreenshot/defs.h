#pragma once

#include "stdafx.h"
#include "MsfScreenShot.h"
#include "DXGIManager.h"

#define CHECK_HR(x) do { HRESULT hr = x; if (FAILED(hr)) { return hr;} } while (0) // do nothing for now
#define CLOSE_HANDLE( h )	if( h ) { CloseHandle( h ); h = NULL; }

#define BREAK_ON_ERROR( str ) { dwResult = GetLastError(); printf( "%s. error=%d", str, dwResult ); break; }
#define BREAK_WITH_ERROR( str, err ) { dwResult = err; printf( "%s. error=%d", str, dwResult ); break; }

//enum ssFormat {BMP, JPG, PNG} x = BMP;
//typedef GUID_ContainerFormatBmp		SSBMP;
//typedef GUID_ContainerFormatJpeg	SSJPG;
//typedef GUID_ContainerFormatPng		SSPNG;
