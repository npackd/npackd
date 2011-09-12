#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>

#include "minidumper.h"

// see http://stackoverflow.com/questions/1547211/how-to-create-minidump-for-my-process-when-it-crashes

LPCSTR MiniDumper::m_szAppName;

MiniDumper::MiniDumper(LPCSTR szAppName)
{
    // if this assert fires then you have two instances of MiniDumper
    // which is not allowed
    assert( m_szAppName==NULL );

    m_szAppName = szAppName ? strdup(szAppName) : "Application";

    ::SetUnhandledExceptionFilter( TopLevelFilter );
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
    LONG retval = EXCEPTION_CONTINUE_SEARCH;

    // find a better value for your app
    // HWND hParent = NULL;

    // firstly see if dbghelp.dll is around and has the function we need
    // look next to the EXE first, as the one in System32 might be old
    // (e.g. Windows 2000)
    HMODULE hDll = NULL;
    char szDbgHelpPath[_MAX_PATH];

    if (GetModuleFileNameA( NULL, szDbgHelpPath, _MAX_PATH ))
    {
        char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
        if (pSlash)
        {
            _tcscpy( pSlash+1, "DBGHELP.DLL" );
            hDll = ::LoadLibraryA( szDbgHelpPath );
        }
    }

    if (hDll==NULL)
    {
        // load any version we can
        hDll = ::LoadLibraryA( "DBGHELP.DLL" );
    }

    const char* szResult = NULL;

    if (hDll)
    {
        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
        if (pDump)
        {
            char szDumpPath[_MAX_PATH];
            char szScratch [_MAX_PATH];

            // work out a good place for the dump file
            if (!GetTempPathA( _MAX_PATH, szDumpPath ))
                _tcscpy( szDumpPath, "c:\\temp\\" );

            _tcscat( szDumpPath, m_szAppName );
            _tcscat( szDumpPath, ".dmp" );

            // ask the user if they want to save a dump file
            if (::MessageBoxA( NULL, "Something bad happened in your program, would you like to save a diagnostic file?", m_szAppName, MB_YESNO )==IDYES)
            {
                // create the file
                HANDLE hFile = ::CreateFileA( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL );

                if (hFile!=INVALID_HANDLE_VALUE)
                {
                    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                    ExInfo.ThreadId = ::GetCurrentThreadId();
                    ExInfo.ExceptionPointers = pExceptionInfo;
                    ExInfo.ClientPointers = NULL;

                    // write the dump
                    BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
                    if (bOK)
                    {
                        sprintf( szScratch, "Saved dump file to '%s'", szDumpPath );
                        szResult = szScratch;
                        retval = EXCEPTION_EXECUTE_HANDLER;
                    }
                    else
                    {
                        sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, (int) GetLastError() );
                        szResult = szScratch;
                    }
                    ::CloseHandle(hFile);
                }
                else
                {
                    sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, (int) GetLastError() );
                    szResult = szScratch;
                }
            }
        }
        else
        {
            szResult = "DBGHELP.DLL too old";
        }
    }
    else
    {
        szResult = "DBGHELP.DLL not found";
    }

    if (szResult)
        ::MessageBoxA( NULL, szResult, m_szAppName, MB_OK );

    return retval;
}
