#include <windows.h>
#include <iostream>
#include <vector>

void MonitorDirectoryChanges(const wchar_t* directory) {
    HANDLE hDir = CreateFileW(
        directory,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open directory" << std::endl;
        return;
    }

    const DWORD bufferSize = 4096;
    std::vector<BYTE> buffer(bufferSize);

    DWORD dwBytesReturned;
    BOOL result;

    while (true) {
        result = ReadDirectoryChangesW(
            hDir,
            buffer.data(),
            bufferSize,
            TRUE, // Watching subtree
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION |
                FILE_NOTIFY_CHANGE_SECURITY,
            &dwBytesReturned,
            NULL,
            NULL
        );

        if (!result) {
            std::cerr << "ReadDirectoryChangesW failed" << std::endl;
            break;
        }

        // Process the changes in buffer here
        // For example, you can parse the buffer to find out what file operation occurred

        // For demonstration purposes, let's just print the buffer
        FILE_NOTIFY_INFORMATION* pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
        while (pInfo) {
            std::wstring filename(pInfo->FileName, pInfo->FileNameLength / sizeof(wchar_t));
            switch(pInfo->Action){
            case FILE_ACTION_ADDED:
                std::wcout << L"Added: " << filename << std::endl;
            case FILE_ACTION_REMOVED:
                std::wcout << L"Removed: " << filename << std::endl;
            case FILE_ACTION_MODIFIED:
                std::wcout << L"Modified: " << filename << std::endl;
            case FILE_ACTION_RENAMED_OLD_NAME:
                std::wcout << L"Renamed (old name): " << filename << std::endl;
            case FILE_ACTION_RENAMED_NEW_NAME:
                std::wcout << L"Renamed: " << filename << std::endl;
            }

            if (pInfo->NextEntryOffset == 0)
                break;
            pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(pInfo) + pInfo->NextEntryOffset);
        }
    }

    CloseHandle(hDir);
}

int main() {
    const wchar_t* directoryToMonitor = L"C:"; // change this
    MonitorDirectoryChanges(directoryToMonitor);
    return 0;
}
