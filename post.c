#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <wchar.h>

#pragma comment(lib, "winhttp.lib")  // 自动链接 WinHTTP 库

char * build_json(char * machine_id, char *hostname, char *os_name, char *cpu_brand, long sec, long nsec, char *version) {
    int len  = snprintf(NULL, 0,
        "{"        
        "\"uuid\": \"%s\","
        "\"info\": {"
            "\"hostname\": \"%s\","
            "\"os\": \"%s\","
            "\"cpu\": \"%s\""
        "},"
        "\"timespec\": {"
            "\"sec\": %lu,"
            "\"nanosec\": %lu"
        "},"
        "\"version\": \"%s\""
        "}",
        machine_id,
        hostname, 
        os_name,
        cpu_brand,        
        sec,
        nsec,
        version
    );
    if (len < 0) {
        return NULL;
    }
    char *json = (char *)malloc(len + 1); // 分配内存，多分配一个字节用于'\0'
    if (json == NULL) {
        return NULL; // 内存分配失败
    } 
    snprintf(json, len + 1,        
        "{"
        "\"uuid\": \"%s\","
        "\"info\": {"
            "\"hostname\": \"%s\","
            "\"os\": \"%s\","
            "\"cpu\": \"%s\""
        "},"
        "\"timespec\": {"
            "\"sec\": %lu,"
            "\"nanosec\": %lu"
        "},"
        "\"version\": \"%s\""
        "}",
        machine_id,
        hostname, 
        os_name,
        cpu_brand,        
        sec,
        nsec,
        version
    );
    return json;
}

int postmark(char * json) {
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // 定义 JSON 数据（UTF-8 格式）
    
    DWORD dataLength = (DWORD)strlen(json);

    // 1. 初始化 WinHTTP 会话
    hSession = WinHttpOpen(
        L"WinHTTP_JSON_Poster/1.0",  // 用户代理标识
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        printf("WinHttpOpen 失败: %lu\n", GetLastError());
        goto cleanup;
    }

    // 2. 连接到目标服务器（假设使用 HTTPS）
    hConnect = WinHttpConnect(
        hSession,
        L"127.0.0.1",  // 目标域名
        12440, // 目标端口
        0
    );
    if (!hConnect) {
        printf("WinHttpConnect 失败: %lu\n", GetLastError());
        goto cleanup;
    }

    // 3. 创建 HTTP 请求句柄
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",          // 方法
        L"/mark",         // 路径
        NULL,             // 协议版本（默认 HTTP/1.1）
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
        // WINHTTP_FLAG_SECURE  // 启用 HTTPS
    );
    if (!hRequest) {
        printf("WinHttpOpenRequest 失败: %lu\n", GetLastError());
        goto cleanup;
    }

    // 4. 设置 HTTP 头
    LPCWSTR headers = L"Content-Type: application/json\r\n";
    if (!WinHttpAddRequestHeaders(
        hRequest,
        headers,
        (DWORD)-1L,  // 自动计算头长度
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
    )) {
        printf("WinHttpAddRequestHeaders 失败: %lu\n", GetLastError());
    }

    // 5. 发送请求
    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)json,  // POST 数据
        dataLength,
        dataLength,
        0
    )) {
        printf("WinHttpSendRequest 失败: %lu\n", GetLastError());
        goto cleanup;
    }

    // 6. 接收响应
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        printf("WinHttpReceiveResponse 失败: %lu\n", GetLastError());
        goto cleanup;
    }

    // 7. 读取响应数据
    DWORD dwSize = 0, dwDownloaded = 0;
    LPSTR pBuffer = NULL;
    do {
        // 检查可用数据大小
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            printf("WinHttpQueryDataAvailable 失败: %lu\n", GetLastError());
            break;
        }
        if (dwSize == 0) break;

        // 分配缓冲区
        pBuffer = (LPSTR)malloc(dwSize + 1);
        if (!pBuffer) {
            printf("内存分配失败\n");
            break;
        }

        // 读取数据块
        if (!WinHttpReadData(hRequest, pBuffer, dwSize, &dwDownloaded)) {
            printf("WinHttpReadData 失败: %lu\n", GetLastError());
            free(pBuffer);
            break;
        }
        pBuffer[dwDownloaded] = '\0';
        printf("\nhttp://musashi/result/%s\n", pBuffer);
        free(pBuffer);
    } while (dwSize > 0);

    // 8. 获取 HTTP 状态码
    DWORD statusCode = 0;
    DWORD dwStatusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &dwStatusCodeSize,
        WINHTTP_NO_HEADER_INDEX
    );
    // printf("HTTP 状态码: %lu\n", statusCode);

cleanup:
    // 清理资源
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return 0;
}


