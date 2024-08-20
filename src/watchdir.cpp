#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <wchar.h>
#include <afx.h>
#include <filesystem>

#include "headlinereader.h"

namespace fs = std::filesystem;

static bool HLTXT = false;

void WatchDirectory(LPTSTR);
std::string w2str(std::wstring filen);
void iteratefiles(LPTSTR lpDir);
static int fail() {
    _tprintf(TEXT("headlinereader -a C:\\...\\scans: run on all files in dir\n"
        "OR\n"
        "headlinereader -w C:\\...\\scans: watch directory\n"
        "OR\n"
        "headlinereader -i news.jpg C:\\...\\scans: run on named file in dir\n"));
    return 1;
};

int _tmain(int argc, TCHAR* argv[]) 
{
    //headlinereader -a C:\...\scans: run on all files in dir
    //headlinereader -w C:\...\scans: watch directory
    //headlinereader -i news.jpg C:\...\scans: run on named file in dir
    fs::path dirpath;

    switch (argc) 
    {
    case 3:
        dirpath = fs::path(w2str(argv[2]));
        if (wcscmp(_T("-a"), argv[1]) == 0)
        {
            fs::current_path(dirpath);
            HLTXT = true;
            iteratefiles(argv[2]);
        }
        else if (wcscmp(_T("-w"), argv[1]) == 0)
        {
            fs::current_path(dirpath);
            WatchDirectory(argv[2]);
        }
        else return fail();
        break;
    case 4:
        if (wcscmp(_T("-i"), argv[1]) == 0)
            get_headline(w2str(argv[2]), w2str(argv[3]), HLTXT);
        else return fail();
        break;
    default:
        return fail();
        break;
    }

}

void iteratefiles(LPTSTR lpDir)
{
    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];
    _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);
    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';
    std::string name;
    std::string dirname = w2str(lpDir);
    CFileFind finder;
    DeleteFile(_T("headlines.txt"));
    CreateFile(_T("headlines.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    finder.FindFile(_T("*.jpg"));
    while (finder.FindNextFile())
    {
        name = CW2A(finder.GetFileName().GetString());
        get_headline(name, dirname, HLTXT);
    }
    name = CW2A(finder.GetFileName().GetString());
    get_headline(name, dirname, HLTXT);
}

std::string processName(LPTSTR lpDir)
{
    FILETIME bestDate = { 0, 0 };
    FILETIME curDate;
    std::string name;
    std::string dirname = w2str(lpDir);
    CFileFind finder;
    finder.FindFile(_T("*.jpg"));
    while (finder.FindNextFile())
    {
        finder.GetCreationTime(&curDate);

        if (CompareFileTime(&curDate, &bestDate) > 0)
        {
            bestDate = curDate;
            name = CW2A(finder.GetFileName().GetString());
        }
    }
    return name;
}

void WatchDirectory(LPTSTR lpDir)
{
    DWORD dwWaitStatus;
    HANDLE dwChangeHandle;
    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];

    _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';

    // Watch the directory for file creation and deletion. 

    dwChangeHandle = FindFirstChangeNotification(
        lpDir,                         // directory to watch 
        FALSE,                         // do not watch subtree 
        FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

    if (dwChangeHandle == INVALID_HANDLE_VALUE)
    {
        printf("\n ERROR: FindFirstChangeNotification function failed.\n");
        ExitProcess(GetLastError());
    }

    // Make a final validation check on our handles.

    if (dwChangeHandle == NULL)
    {
        printf("\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n");
        ExitProcess(GetLastError());
    }

    // Change notification is set. Now wait on both notification 
    // handles and refresh accordingly. 

    while (TRUE)
    {
        // Wait for notification.

        dwWaitStatus = WaitForSingleObject(dwChangeHandle, INFINITE);

        if (dwWaitStatus == WAIT_OBJECT_0) 
        {
            get_headline(processName(lpDir), w2str(lpDir), HLTXT);
            if (FindNextChangeNotification(dwChangeHandle) == FALSE)
            {
                printf("\n ERROR: FindNextChangeNotification function failed.\n");
                ExitProcess(GetLastError());
            }
        }

    }
}

std::string w2str(std::wstring filen)
{
    std::string str(filen.length(), 0);
    std::transform(filen.begin(), filen.end(), str.begin(), [](wchar_t c) {
        return (char)c;
        });
    return str;
}