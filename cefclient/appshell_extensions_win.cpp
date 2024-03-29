/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 */ 
#include "appshell_extensions_platform.h"
#include "client_handler.h"

#include <algorithm>
#include <CommDlg.h>
#include <Psapi.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <sys/stat.h>

#define CLOSING_PROP L"CLOSING"
#define UNICODE_MINUS 0x2212
#define UNICODE_LEFT_ARROW 0x2190
#define UNICODE_DOWN_ARROW 0x2193


// Forward declarations for functions at the bottom of this file
void ConvertToNativePath(ExtensionString& filename);
void ConvertToUnixPath(ExtensionString& filename);

int ConvertErrnoCode(int errorCode, bool isReading = true);
int ConvertWinErrorCode(int errorCode, bool isReading = true);
static std::wstring GetPathToLiveBrowser();
static bool ConvertToShortPathName(std::wstring & path);
time_t FiletimeToTime(FILETIME const& ft);

extern HINSTANCE hInst;

extern HACCEL hAccelTable;
extern std::wstring gFilesToOpen;

// constants
#define MAX_LOADSTRING 100

static bool ConvertToShortPathName(std::wstring & path)
{
    DWORD shortPathBufSize = _MAX_PATH+1;
    WCHAR shortPathBuf[_MAX_PATH+1];
    DWORD finalShortPathSize = ::GetShortPathName(path.c_str(), shortPathBuf, shortPathBufSize);
    if( finalShortPathSize == 0 ) {
        return false;
    }
        
    path.assign(shortPathBuf, finalShortPathSize);
    return true;
}
int32 OpenURLInDefaultBrowser(ExtensionString url)
{
    DWORD result = (DWORD)ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    // If the result > 32, the function suceeded. If the result is <= 32, it is an
    // error code.
    if (result <= 32)
        return ConvertWinErrorCode(result);

    return NO_ERROR;
}


static int CALLBACK SetInitialPathCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (BFFM_INITIALIZED == uMsg && NULL != lpData)
    {
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}


int32 ShowOpenDialog(bool allowMultipleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    wchar_t szFile[MAX_PATH];
    szFile[0] = 0;

    // TODO (issue #64) - This method should be using IFileDialog instead of the
    /* outdated SHGetPathFromIDList and GetOpenFileName.
       
    Useful function to parse fileTypesStr:
    template<class T>
    int inline findAndReplaceString(T& source, const T& find, const T& replace)
    {
    int num=0;
    int fLen = find.size();
    int rLen = replace.size();
    for (int pos=0; (pos=source.find(find, pos))!=T::npos; pos+=rLen)
    {
    num++;
    source.replace(pos, fLen, replace);
    }
    return num;
    }
    */

    // SHBrowseForFolder can handle Windows path only, not Unix path.
    // ofn.lpstrInitialDir also needs Windows path on XP and not Unix path.
    ConvertToNativePath(initialDirectory);

    if (chooseDirectory) {
        BROWSEINFO bi = {0};
        bi.hwndOwner = GetActiveWindow();
        bi.lpszTitle = title.c_str();
        bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
        bi.lpfn = SetInitialPathCallback;
        bi.lParam = (LPARAM)initialDirectory.c_str();

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != 0) {
            if (SHGetPathFromIDList(pidl, szFile)) {
                // Add directory path to the result
                ExtensionString pathName(szFile);
                ConvertToUnixPath(pathName);
                selectedFiles->SetString(0, pathName);
            }
            IMalloc* pMalloc = NULL;
            SHGetMalloc(&pMalloc);
            if (pMalloc) {
                pMalloc->Free(pidl);
                pMalloc->Release();
            }
        }
    } else {
        OPENFILENAME ofn;

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.hwndOwner = GetActiveWindow();
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;

        // TODO (issue #65) - Use passed in file types. Note, when fileTypesStr is null, all files should be shown
        /* findAndReplaceString( fileTypesStr, std::string(" "), std::string(";*."));
        LPCWSTR allFilesFilter = L"All Files\0*.*\0\0";*/

        ofn.lpstrFilter = L"All Files\0*.*\0Web Files\0*.js;*.css;*.htm;*.html\0\0";
           
        ofn.lpstrInitialDir = initialDirectory.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        if (allowMultipleSelection)
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if (GetOpenFileName(&ofn)) {
            if (allowMultipleSelection) {
                // Multiple selection encodes the files differently

                // If multiple files are selected, the first null terminator
                // signals end of directory that the files are all in
                std::wstring dir(szFile);

                // Check for two null terminators, which signal that only one file
                // was selected
                if (szFile[dir.length() + 1] == '\0') {
                    ExtensionString filePath(dir);
                    ConvertToUnixPath(filePath);
                    selectedFiles->SetString(0, filePath);
                } else {
                    // Multiple files are selected
                    wchar_t fullPath[MAX_PATH];
                    for (int i = (dir.length() + 1), fileIndex = 0; ; fileIndex++) {
                        // Get the next file name
                        std::wstring file(&szFile[i]);

                        // Two adjacent null characters signal the end of the files
                        if (file.length() == 0)
                            break;

                        // The filename is relative to the directory that was specified as
                        // the first string
                        if (PathCombine(fullPath, dir.c_str(), file.c_str()) != NULL) {
                            ExtensionString filePath(fullPath);
                            ConvertToUnixPath(filePath);
                            selectedFiles->SetString(fileIndex, filePath);
                        }

                        // Go to the start of the next file name
                        i += file.length() + 1;
                    }
                }

            } else {
                // If multiple files are not allowed, add the single file
                selectedFiles->SetString(0, szFile);
            }
        }
    }

    return NO_ERROR;
}

int32 IsNetworkDrive(ExtensionString path, bool& isRemote)
{
    if (path.length() == 0) {
        return ERR_INVALID_PARAMS;
    }

    ExtensionString drive = path.substr(0, path.find('/') + 1);
    isRemote = GetDriveType(drive.c_str()) == DRIVE_REMOTE;

    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    if (path.length() && path[path.length() - 1] != '/')
        path += '/';

    path += '*';

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(path.c_str(), &ffd);

    std::vector<ExtensionString> resultFiles;
    std::vector<ExtensionString> resultDirs;

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Ignore '.' and '..' and system files
            if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..") ||
                (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
                continue;

            // Collect file and directory names separately
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                resultDirs.push_back(ExtensionString(ffd.cFileName));
            } else {
                resultFiles.push_back(ExtensionString(ffd.cFileName));
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    } 
    else {
        return ConvertWinErrorCode(GetLastError());
    }

    // On Windows, list directories first, then files
    size_t i, total = 0;
    for (i = 0; i < resultDirs.size(); i++)
        directoryContents->SetString(total++, resultDirs[i]);
    for (i = 0; i < resultFiles.size(); i++)
        directoryContents->SetString(total++, resultFiles[i]);

    return NO_ERROR;
}

int32 MakeDir(ExtensionString path, int32 mode)
{
    // TODO (issue #1759): honor mode
    ConvertToNativePath(path);
    int err = SHCreateDirectoryEx(NULL, path.c_str(), NULL);

    return ConvertWinErrorCode(err);
}

int32 Rename(ExtensionString oldName, ExtensionString newName)
{
    if (!MoveFile(oldName.c_str(), newName.c_str()))
        return ConvertWinErrorCode(GetLastError());

    return NO_ERROR;
}

int32 GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES) {
        return ConvertWinErrorCode(GetLastError()); 
    }

    isDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    WIN32_FILE_ATTRIBUTE_DATA   fad;
    if (!GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad)) {
        return ConvertWinErrorCode(GetLastError());
    }

    modtime = FiletimeToTime(fad.ftLastWriteTime);

    return NO_ERROR;
}

int32 ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{
    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    DWORD dwAttr;
    dwAttr = GetFileAttributes(filename.c_str());
    if (INVALID_FILE_ATTRIBUTES == dwAttr)
        return ConvertWinErrorCode(GetLastError());

    if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        return ERR_CANT_READ;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int32 error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError()); 

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwBytesRead;
    char* buffer = (char*)malloc(dwFileSize);
    if (buffer && ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
        contents = std::string(buffer, dwFileSize);
    }
    else {
        if (!buffer)
            error = ERR_UNKNOWN;
        else
            error = ConvertWinErrorCode(GetLastError());
    }
    CloseHandle(hFile);
    if (buffer)
        free(buffer);

    return error; 
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    if (encoding != L"utf8")
        return ERR_UNSUPPORTED_ENCODING;

    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesWritten;
    int error = NO_ERROR;

    if (INVALID_HANDLE_VALUE == hFile)
        return ConvertWinErrorCode(GetLastError(), false); 

    // TODO (issue 67) -  Should write to temp file and handle encoding
    if (!WriteFile(hFile, contents.c_str(), contents.length(), &dwBytesWritten, NULL)) {
        error = ConvertWinErrorCode(GetLastError(), false);
    }

    CloseHandle(hFile);
    return error;
}

int32 SetPosixPermissions(ExtensionString filename, int32 mode)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES)
        return ERR_NOT_FOUND;

    if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return NO_ERROR;

    bool write = (mode & 0200) != 0; 
    bool read = (mode & 0400) != 0;
    int mask = (write ? _S_IWRITE : 0) | (read ? _S_IREAD : 0);

    if (_wchmod(filename.c_str(), mask) == -1) {
        return ConvertErrnoCode(errno); 
    }

    return NO_ERROR;
}

int32 DeleteFileOrDirectory(ExtensionString filename)
{
    DWORD dwAttr = GetFileAttributes(filename.c_str());

    if (dwAttr == INVALID_FILE_ATTRIBUTES)
        return ERR_NOT_FOUND;

    if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return ERR_NOT_FILE;

    if (!DeleteFile(filename.c_str()))
        return ConvertWinErrorCode(GetLastError());

    return NO_ERROR;
}


void OnBeforeShutdown()
{
    
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        HWND browserHwnd = browser->GetHost()->GetWindowHandle();
        SetProp(browserHwnd, CLOSING_PROP, (HANDLE)1);
        browser->GetHost()->CloseBrowser();
    }
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        HWND hwnd = browser->GetHost()->GetWindowHandle();
        if (hwnd) 
            ::BringWindowToTop(hwnd);
    }
}

void ConvertToNativePath(ExtensionString& filename)
{
    // Convert '/' to '\'
    replace(filename.begin(), filename.end(), '/', '\\');
}

void ConvertToUnixPath(ExtensionString& filename)
{
    // Convert '\\' to '/'
    replace(filename.begin(), filename.end(), '\\', '/');
}

// Maps errors from errno.h to the brackets error codes
// found in brackets_extensions.js
int ConvertErrnoCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case EINVAL:
        return ERR_INVALID_PARAMS;
    case ENOENT:
        return ERR_NOT_FOUND;
    default:
        return ERR_UNKNOWN;
    }
}

// Maps errors from  WinError.h to the brackets error codes
// found in brackets_extensions.js
int ConvertWinErrorCode(int errorCode, bool isReading)
{
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case ERROR_PATH_NOT_FOUND:
    case ERROR_FILE_NOT_FOUND:
        return ERR_NOT_FOUND;
    case ERROR_ACCESS_DENIED:
        return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    case ERROR_WRITE_PROTECT:
        return ERR_CANT_WRITE;
    case ERROR_HANDLE_DISK_FULL:
        return ERR_OUT_OF_SPACE;
    case ERROR_ALREADY_EXISTS:
        return ERR_FILE_EXISTS;
    default:
        return ERR_UNKNOWN;
    }
}

/**
 * Convert a FILETIME (number of 100 nanosecond intervals since Jan 1 1601)
 * to time_t (number of seconds since Jan 1 1970)
 */
time_t FiletimeToTime(FILETIME const& ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // Convert the FILETIME from 100 nanosecond intervals into seconds
    // and then subtract the number of seconds between 
    // Jan 1 1601 and Jan 1 1970

    const int64 NANOSECOND_INTERVALS = 10000000ULL;
    const int64 SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970 = 11644473600ULL;

    return ull.QuadPart / NANOSECOND_INTERVALS - SECONDS_FROM_JAN_1_1601_TO_JAN_1_1970;
}

int32 ShowFolderInOSWindow(ExtensionString pathname) {
    ShellExecute(NULL, L"open", pathname.c_str(), NULL, NULL, SW_SHOWDEFAULT);
    return NO_ERROR;
}

int32 GetPendingFilesToOpen(ExtensionString& files) {
    files = gFilesToOpen;
    ConvertToUnixPath(files);
    gFilesToOpen = L"";
    return NO_ERROR;
}

