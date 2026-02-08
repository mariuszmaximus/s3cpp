#include <curl/curl.h>
#include <curl/easy.h>
#include <expected>
#include <format>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>

// Route to its HttpMethod
std::expected<HttpResponse, std::string> HttpRequest::execute() {
    switch (this->http_method_) {
    case HttpMethod::Get:
        return client_.execute_get(*this);
    case HttpMethod::Head:
        return client_.execute_head(*this);
    default:
        return std::unexpected<std::string>("No matching enum Http Method");
    }
}

std::expected<HttpResponse, std::string> HttpBodyRequest::execute() {
    switch (this->http_method_) {
    case HttpMethod::Post:
        return client_.execute_post(*this);
    case HttpMethod::Put:
        return client_.execute_post(*this);
    case HttpMethod::Delete:
        return client_.execute_delete(*this);
    default:
        return std::unexpected<std::string>("No matching enum Http Method");
    }
}

std::expected<HttpResponse, std::string> HttpClient::execute_get(HttpRequest& request) {
    if (!curl_handle) {
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        return std::unexpected<std::string>("cURL handle is invalid");
    }
    std::string body_buf;
    std::map<std::string, std::string, LowerCaseCompare> headers_buf;
    std::string error_buf;

    // TODO(cristian): from libcurl docs, they state that each curl handle has
    // "sticky" params, this is why we are resetting at each get request
    // However, I think we should only do this when moving a new handle the only
    // thing that will change is the URL from now
    //
    // curl_easy_reset(curl_handle);

    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
    // body callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_buf);
    // headers callback
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers_buf);

    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

    // merge client and request headers
    // https://stackoverflow.com/questions/34321719
    auto headers = request.getHeaders();
    headers.insert(this->getHeaders().begin(), this->getHeaders().end());
    struct curl_slist* list = NULL;
    for (const auto& [k, v] : headers) {
        list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    CURLcode code = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if (code != CURLE_OK) {
        return std::unexpected<std::string>(std::format("libcurl error: {}", curl_easy_strerror(code)));
    }

    // get HTTP code
    long response_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

    return HttpResponse(static_cast<int>(response_code), std::move(body_buf),
        std::move(headers_buf));
}

std::expected<HttpResponse, std::string> HttpClient::execute_head(HttpRequest& request) {
    if (!curl_handle) {
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        return std::unexpected<std::string>("cURL handle is invalid");
    }
    std::map<std::string, std::string, LowerCaseCompare> headers_buf;
    std::string error_buf;

    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers_buf);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);

    // merge client and request headers
    // https://stackoverflow.com/questions/34321719
    auto headers = request.getHeaders();
    headers.insert(this->getHeaders().begin(), this->getHeaders().end());
    struct curl_slist* list = NULL;
    for (const auto& [k, v] : headers) {
        list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    CURLcode code = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if (code != CURLE_OK) {
        return std::unexpected<std::string>(std::format("libcurl error: {}", curl_easy_strerror(code)));
    }

    // get HTTP code
    long response_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

    return HttpResponse(static_cast<int>(response_code), std::move(headers_buf));
}

std::expected<HttpResponse, std::string> HttpClient::execute_post(HttpBodyRequest& request) {
    if (!curl_handle) {
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        return std::unexpected<std::string>("cURL handle is invalid");
    }
    std::string body_buf;
    std::map<std::string, std::string, LowerCaseCompare> headers_buf;
    std::string error_buf;

    // TODO(cristian): from libcurl docs, they state that each curl handle has
    // "sticky" params, this is why we are resetting at each get request
    // However, I think we should only do this when moving a new handle the only
    // thing that will change is the URL from now
    //
    // curl_easy_reset(curl_handle);

    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
    // body callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_buf);
    // headers callback
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers_buf);
    // post/put body
    if (request.getHttpMethod() == HttpMethod::Put) {
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
    } else {
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    }
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request.getBody().c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, request.getBody().size());

    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

    // merge client and request headers
    // https://stackoverflow.com/questions/34321719
    auto headers = request.getHeaders();
    headers.insert(this->getHeaders().begin(), this->getHeaders().end());
    struct curl_slist* list = NULL;
    for (const auto& [k, v] : headers) {
        list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    CURLcode code = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if (code != CURLE_OK) {
        return std::unexpected<std::string>(std::format("libcurl error: {}", curl_easy_strerror(code)));
    }

    // get HTTP code
    long response_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

    return HttpResponse(static_cast<int>(response_code), std::move(body_buf),
        std::move(headers_buf));
}

std::expected<HttpResponse, std::string> HttpClient::execute_delete(HttpBodyRequest& request) {
    if (!curl_handle) {
        // this can happen both when cURL handle is not initialized or when it
        // is invalidated in the HTTPClient copy constructor
        return std::unexpected<std::string>("cURL handle is invalid");
    }
    std::string body_buf;
    std::map<std::string, std::string, LowerCaseCompare> headers_buf;
    std::string error_buf;

    // TODO(cristian): from libcurl docs, they state that each curl handle has
    // "sticky" params, this is why we are resetting at each get request
    // However, I think we should only do this when moving a new handle the only
    // thing that will change is the URL from now
    //
    // curl_easy_reset(curl_handle);

    curl_easy_setopt(curl_handle, CURLOPT_URL, request.getURL().c_str());
    // body callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_buf);
    // headers callback
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers_buf);
    // delete may have or not have a body
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    if (request.getBody() != "") // use std::optional
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request.getBody().c_str());

    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.getTimeout());

    // merge client and request headers
    // https://stackoverflow.com/questions/34321719
    auto headers = request.getHeaders();
    headers.insert(this->getHeaders().begin(), this->getHeaders().end());
    struct curl_slist* list = NULL;
    for (const auto& [k, v] : headers) {
        list = curl_slist_append(list, std::format("{}: {}", k, v).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    CURLcode code = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if (code != CURLE_OK) {
        return std::unexpected<std::string>(std::format("libcurl error: {}", curl_easy_strerror(code)));
    }

    // get HTTP code
    long response_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &response_code);

    return HttpResponse(static_cast<int>(response_code), std::move(body_buf),
        std::move(headers_buf));
}

size_t HttpClient::write_callback(char* ptr, size_t size, size_t nmemb,
    void* userdata) {
    std::string* buffer = static_cast<std::string*>(userdata);
    size_t total_size = size * nmemb;
    buffer->append(ptr, total_size);
    return total_size;
}

size_t HttpClient::header_callback(char* buffer, size_t size, size_t nitems,
    void* userdata) {
    // from libcurl docs:
    // The header callback is called once for each header and
    // only complete header lines are passed on to the callback.
    auto headers = static_cast<std::map<std::string, std::string>*>(userdata);
    size_t total_size = size * nitems;

    std::string line(buffer, total_size);

    if (line.find(":") == std::string::npos || line == "\r\n" || line == "\n") {
        return total_size;
    }

    size_t separator = line.find(":");
    if (separator != std::string::npos) {
        std::string k = line.substr(0, separator);
        std::string v = line.substr(separator + 2); // `:_<here>` advance 2
        v.erase(v.find_last_not_of("\r\n") + 1); // .strip()
        (*headers)[k] = v;
    }

    return total_size;
}
