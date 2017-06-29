#pragma once

#include "stdafx.h"
#include "MsfScreenShot.h"
#include "DXGIManager.h"
#include "defs.h"

MsfScreenShot::MsfScreenShot()
{
	CoInitialize(NULL);
	m_DXGIManager.SetCaptureSource(CSDesktop);
	m_spWICFactory.CoCreateInstance(CLSID_WICImagingFactory);
	m_DXGIManager.GetOutputRect(m_rcDim);
	m_dwWidth = m_rcDim.right - m_rcDim.left;
	m_dwHeight = m_rcDim.bottom - m_rcDim.top;
}

MsfScreenShot::~MsfScreenShot()
{
	delete[] m_pBuf;
}

HRESULT MsfScreenShot::InitializeRawDesktopBitmap()
{
	DWORD m_dwBufSize = m_dwWidth*m_dwHeight * 4;
	m_pBuf = new BYTE[m_dwBufSize];
	HRESULT hr;

	int i = 0;
	do
	{
		hr = m_DXGIManager.GetOutputBits(m_pBuf, m_rcDim);
		i++;
	} while (hr == DXGI_ERROR_WAIT_TIMEOUT || i < 2);

	if (FAILED(hr))
		return hr;

	return 0;
}

HRESULT MsfScreenShot::EncodeData(BYTE **out, DWORD *size, GUID encode_format)
{
	CComPtr<IStream> pStream(SHCreateMemStream(NULL, 0));
	{
		CComPtr<IWICBitmapEncoder> pEncoder;
		CHECK_HR(m_spWICFactory->CreateEncoder(encode_format, NULL, &pEncoder));
		CHECK_HR(pEncoder->Initialize(pStream, WICBitmapEncoderCacheOption::WICBitmapEncoderNoCache));

		CComPtr<IWICBitmapFrameEncode> pFrame;
		CHECK_HR(pEncoder->CreateNewFrame(&pFrame, NULL));
		CHECK_HR(pFrame->Initialize(NULL));

		CComPtr<IWICBitmap> pBitmap;
		CHECK_HR(m_spWICFactory->CreateBitmapFromMemory(m_dwWidth, m_dwHeight, GUID_WICPixelFormat32bppBGRA, m_dwWidth * 4, m_dwWidth * m_dwHeight * 4, m_pBuf, &pBitmap));
		CHECK_HR(pFrame->WriteSource(pBitmap, NULL));
		CHECK_HR(pFrame->Commit());
		CHECK_HR(pEncoder->Commit());
	}

	STATSTG statstg;
	CHECK_HR(pStream->Stat(&statstg, STATFLAG_NONAME));

	LARGE_INTEGER l;
	l.QuadPart = 0;
	CHECK_HR(pStream->Seek(l, STREAM_SEEK_SET, NULL));

	*size = statstg.cbSize.LowPart;
	*out = new BYTE[*size];
	ULONG dummy;

	CHECK_HR(pStream->Read(*out, statstg.cbSize.LowPart, &dummy));

	return S_OK;
}

UINT MsfScreenShot::GetWidth()
{
	return this->m_dwWidth;
}

UINT MsfScreenShot::GetHeight()
{
	return this->m_dwHeight;
}

DWORD MsfScreenShot::SendScreenShot(char * cpNamedPipe, BYTE * pJpegBuffer, DWORD dwJpegSize)
{
	DWORD dwResult = ERROR_ACCESS_DENIED;
	HANDLE hPipe = NULL;
	DWORD dwWritten = 0;
	DWORD dwTotal = 0;

	do
	{
		hPipe = CreateFileA(cpNamedPipe, GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hPipe)
			BREAK_ON_ERROR("[SCREENSHOT] screenshot_send. CreateFileA failed");

		if (!WriteFile(hPipe, (LPCVOID)&dwJpegSize, sizeof(DWORD), &dwWritten, NULL))
			BREAK_ON_ERROR("[SCREENSHOT] screenshot_send. WriteFile JPEG length failed");

		if (!dwJpegSize || !pJpegBuffer)
			BREAK_WITH_ERROR("[SCREENSHOT] screenshot_send. No JPEG to transmit.", ERROR_BAD_LENGTH);

		while (dwTotal < dwJpegSize)
		{
			if (!WriteFile(hPipe, (LPCVOID)(pJpegBuffer + dwTotal), (dwJpegSize - dwTotal), &dwWritten, NULL))
				break;
			dwTotal += dwWritten;
		}

		if (dwTotal != dwJpegSize)
			BREAK_WITH_ERROR("[SCREENSHOT] screenshot_send. dwTotal != dwJpegSize", ERROR_BAD_LENGTH);

		dwResult = ERROR_SUCCESS;

	} while (0);

	CLOSE_HANDLE(hPipe);

	return dwResult;
}