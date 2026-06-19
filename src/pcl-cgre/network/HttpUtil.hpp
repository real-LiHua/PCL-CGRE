#pragma once

#include <string>
#include <mutex>

#include <curl/curl.h>

#include "core/Log.hpp"

namespace pcl::util {

/** Timeout for all synchronous HTTP requests (seconds).
 *
 *  Aligns with REQUEST_TIMEOUT_S in ResourceFetcher.cpp icon loader.
 *  Without a timeout, a stalled TCP connection (e.g. after a period
 *  of inactivity when server-side keepalive has dropped) would block
 *  the worker thread indefinitely, and the UI would appear to freeze
 *  until the system TCP timeout fires (~30–120 s). */
constexpr int HTTP_SYNC_TIMEOUT_S = 10;

namespace {

/** libcurl write callback — appends received data to a std::string. */
inline size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* str = static_cast<std::string*>(userdata);
    str->append(ptr, size * nmemb);
    return size * nmemb;
}

/** Thread-local CURL handle — created once per worker thread and reused
 *  for all subsequent HTTP requests on that thread.  This lets libcurl
 *  keep TCP connections alive (HTTP/1.1 keep-alive) across requests.
 *
 *  The handle is leaked on thread exit, which is acceptable because
 *  worker threads are long-lived (they handle a single fetch-and-die
 *  pattern in McVersionFetcher / ResourceFetcher).
 *
 *  curl_global_init() is called exactly once before the first handle
 *  is created — it is thread-safe per libcurl documentation. */
inline CURL* tl_handle()
{
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });

    static thread_local CURL* handle = []() -> CURL* {
        CURL* h = curl_easy_init();
        if (h) {
            curl_easy_setopt(h, CURLOPT_TIMEOUT, (long)HTTP_SYNC_TIMEOUT_S);
            curl_easy_setopt(h, CURLOPT_USERAGENT, "PCL-CGRE/0.1");
            curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(h, CURLOPT_MAXREDIRS, 5L);
            curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_callback);
        }
        return h;
    }();
    return handle;
}

}  // anonymous namespace

/**
 * Synchronous HTTP GET. Returns the response body as a string,
 * or an empty string on failure. Logs errors via LOG_WARN.
 *
 * Called from background threads — never from the GTK main thread.
 * Uses a thread-local CURL handle for TCP connection reuse.
 */
inline std::string http_get_sync(const char* url)
{
    CURL* curl = tl_handle();
    if (!curl) {
        LOG_WARN("HttpUtil: curl_easy_init failed — no CURL handle");
        return {};
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);  // no extra headers

    std::string result;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOG_WARN("HttpUtil: HTTP error from %s: %s",
                 url, curl_easy_strerror(res));
        return {};
    }

    return result;
}

/**
 * Synchronous HTTP GET with optional API key header (for CurseForge).
 */
inline std::string http_get_sync(const char* url, const char* api_key)
{
    CURL* curl = tl_handle();
    if (!curl) {
        LOG_WARN("HttpUtil: curl_easy_init failed — no CURL handle");
        return {};
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist* headers = nullptr;
    if (api_key && *api_key) {
        std::string header = std::string("x-api-key: ") + api_key;
        headers = curl_slist_append(headers, header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string result;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOG_WARN("HttpUtil: HTTP error from %s: %s",
                 url, curl_easy_strerror(res));
    }

    if (headers)
        curl_slist_free_all(headers);

    return (res == CURLE_OK) ? result : std::string{};
}

}  // namespace pcl::util
