#pragma once

#include "stdafx.h"
#include "DXGIManager.h"

class MsfScreenShot
{
public:
	MsfScreenShot();
	~MsfScreenShot();
	UINT GetWidth();
	UINT GetHeight();
	HRESULT InitializeRawDesktopBitmap();
	HRESULT EncodeData(BYTE **out, DWORD *size, GUID encode_format);
	DWORD SendScreenShot(char * cpNamedPipe, BYTE * pJpegBuffer, DWORD dwJpegSize);

private:
	BYTE* m_pBuf;
	DWORD m_dwWidth;
	DWORD m_dwHeight;
	RECT m_rcDim;
	DXGIManager m_DXGIManager;
	CComPtr<IWICImagingFactory> m_spWICFactory;
};

