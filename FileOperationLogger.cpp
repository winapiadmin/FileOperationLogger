#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fcntl.h>
#include <io.h>

std::mutex mtx;
std::condition_variable cv;
std::queue<std::wstring> logQueue;
bool done = false;

void logWorker() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !logQueue.empty() || done; });

        while (!logQueue.empty()) {
            std::wcout << logQueue.front() << std::endl;
            logQueue.pop();
        }

        if (done && logQueue.empty()) break;
    }
}

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

    // Set console output to UTF-8 to handle Unicode characters
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);

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
        FILE_NOTIFY_INFORMATION* pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
        std::unique_lock<std::mutex> lock(mtx);
        while (pInfo) {
            std::wstring filename(pInfo->FileName, pInfo->FileNameLength / sizeof(wchar_t));
            std::wstring fullPath = std::wstring(directory) + L"\\" + filename;

            switch (pInfo->Action) {
                case FILE_ACTION_ADDED:
                    logQueue.push(L"Added: " + fullPath);
                case FILE_ACTION_REMOVED:
                    logQueue.push(L"Removed: " + fullPath);
                case FILE_ACTION_MODIFIED:
                    logQueue.push(L"Modified: " + fullPath);
                case FILE_ACTION_RENAMED_OLD_NAME:
                    logQueue.push(L"Renamed (old name): " + fullPath);
                case FILE_ACTION_RENAMED_NEW_NAME:
                    logQueue.push(L"Renamed: " + fullPath);
            }

            if (pInfo->NextEntryOffset == 0)
                break;
            pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(pInfo) + pInfo->NextEntryOffset);
        }
        lock.unlock();
        cv.notify_one();
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();

    CloseHandle(hDir);
}

int main() {
    std::thread logger(logWorker);

    const wchar_t* directoryToMonitor = L"C:";
    MonitorDirectoryChanges(directoryToMonitor);

    logger.join();
    return 0;
}
