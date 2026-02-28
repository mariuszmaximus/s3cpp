#ifndef S3CPP_HTTPCLIENT
#define S3CPP_HTTPCLIENT

#include <chrono>
#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include <expected>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace s3cpp {
// Forward declaration
class HttpClient;

enum class HttpMethod { Get, Post, Put, Head, Delete };

struct LowerCaseCompare { // A custom lambda to sort keys alphabetically
  bool operator()(const std::string &a, const std::string &b) const {
    std::string sa = a;
    std::string sb = b;
    std::transform(sa.begin(), sa.end(), sa.begin(), ::tolower);
    std::transform(sb.begin(), sb.end(), sb.begin(), ::tolower);
    return sa < sb;
  }
};

class HttpResponse {
public:
  HttpResponse(int c) : code_(c) {};
  HttpResponse(int c, std::string b) : code_(c), body_(std::move(b)) {};
  HttpResponse(int c, std::map<std::string, std::string, LowerCaseCompare> h)
      : code_(c), headers_(std::move(h)) {};
  HttpResponse(int c, std::string b,
               std::map<std::string, std::string, LowerCaseCompare> h)
      : code_(c), body_(std::move(b)), headers_(std::move(h)) {};

  // Getters
  int status() const { return code_; }
  const std::string &body() const { return body_; }
  const auto &headers() const { return headers_; }

  // Status via code
  bool is_ok() const { return code_ >= 200 && code_ < 300; }
  bool is_redirect() const { return code_ >= 300 && code_ < 400; }
  bool is_client_error() const { return code_ >= 400 && code_ < 500; }
  bool is_server_error() const { return code_ >= 500 && code_ < 600; }

  bool operator==(const HttpResponse &other) const {
    return (code_ == other.code_ && body_ == other.body_ &&
            headers_ == other.headers_);
  }

private:
  int code_;
  std::string body_;
  std::map<std::string, std::string, LowerCaseCompare> headers_;
};

// HttpRequest will handle all the headers and request params
//
// Curiously Recurring Template Pattern (CRTP)
//
// POST/PUT must a have a body member variable that GET/HEAD don't
// we want to do this at compile-time, however, because we are using a fluent
// builder pattern, if we were to do regular inheritance, when using .body() we
// would return a `&HttpBodyRequest : public HttpRequest`, which would not allow
// us to chain useful methods on the HttpRequest class such as .timeout()

template <typename T> class HttpRequestBase {
public:
  HttpRequestBase(HttpClient &client, std::string URL,
                  const HttpMethod &http_method)
      : client_(client), URL_(std::move(URL)),
        http_method_(std::move(http_method)), timeout_(0) {};

  T &timeout(const long long &seconds) {
    timeout_ = std::chrono::seconds(seconds);
    return static_cast<T &>(*this);
  }
  T &timeout(const std::chrono::seconds &seconds) {
    timeout_ = seconds;
    return static_cast<T &>(*this);
  }
  T &header(const std::string &header_, const std::string &value) {
    headers_[header_] = value;
    return static_cast<T &>(*this);
  }

  const std::string &getURL() const { return URL_; }
  const HttpMethod &getHttpMethod() const { return http_method_; }
  const long long getTimeout() const { return timeout_.count(); }
  const std::map<std::string, std::string, LowerCaseCompare> &
  getHeaders() const {
    return headers_;
  }

  // Cannonicalize HTTP verb from the request
  const std::string getHttpMethodStr(const HttpMethod &http_method) const {
    switch (http_method) {
    case HttpMethod::Get:
      return "GET";
    case HttpMethod::Head:
      return "HEAD";
    case HttpMethod::Post:
      return "POST";
    case HttpMethod::Put:
      return "PUT";
    case HttpMethod::Delete:
      return "DELETE";
    default:
      throw std::runtime_error("No known Http Method");
    }
  }

protected:
  HttpClient &client_;
  std::string URL_;
  std::map<std::string, std::string, LowerCaseCompare> headers_;
  std::chrono::seconds timeout_;
  HttpMethod http_method_;
};

// GET/HEAD
class HttpRequest : public HttpRequestBase<HttpRequest> {
public:
  using HttpRequestBase::HttpRequestBase;
  std::expected<HttpResponse, std::string> execute();
};

// POST/PUT
class HttpBodyRequest : public HttpRequestBase<HttpBodyRequest> {
public:
  HttpBodyRequest(HttpClient &client, std::string URL,
                  const HttpMethod &http_method)
      : HttpRequestBase(client, std::move(URL), http_method) {}

  HttpBodyRequest &body(const std::string &data) {
    body_ = std::move(data);
    return (*this);
  }

  const std::string &getBody() const { return body_; }

  std::expected<HttpResponse, std::string> execute();

private:
  std::string body_ = "";
};

// HttpClient should only focus on handling the cURL handle
// and making the request (HttpRequest) and returning HttpResponse
class HttpClient {
  // `execute()` is invoked from the request only
  friend class HttpRequest;
  friend class HttpBodyRequest;

public:
  HttpClient() {
    curl_handle = curl_easy_init();
    if (!curl_handle)
      throw std::runtime_error("Failed to initialize cURL");
    headers_["User-Agent"] = "s3cpp/0.0.0 github.com/ggcr/s3cpp";
  }
  HttpClient(std::unordered_map<std::string, std::string> headers)
      : headers_(std::move(headers)) {
    curl_handle = curl_easy_init();
    if (!curl_handle)
      throw std::runtime_error("Failed to initialize cURL");
    headers_["User-Agent"] = "s3cpp/0.0.0 github.com/ggcr/s3cpp";
  }
  ~HttpClient() {
    if (curl_handle)
      curl_easy_cleanup(curl_handle);
  };

  HttpClient(const HttpClient &) = delete;
  HttpClient &operator=(const HttpClient &) = delete;

  // When transfering ownership we must invalidate the source cURL handle
  HttpClient(HttpClient &&other) : curl_handle(other.curl_handle) {
    other.curl_handle = nullptr;
  }
  HttpClient &operator=(HttpClient &&other) {
    if (this != &other) {
      // cleanup current handle
      if (curl_handle)
        curl_easy_cleanup(curl_handle);
      curl_handle = std::move(other.curl_handle);
      other.curl_handle = nullptr;
    }
    return *this;
  }

  // HTTP GET
  [[nodiscard]] HttpRequest get(const std::string &URL) {
    return HttpRequest{*this, URL, HttpMethod::Get};
  };
  [[nodiscard]] HttpRequest head(const std::string &URL) {
    return HttpRequest{*this, URL, HttpMethod::Head};
  };
  [[nodiscard]] HttpBodyRequest post(const std::string &URL) {
    return HttpBodyRequest{*this, URL, HttpMethod::Post};
  };
  [[nodiscard]] HttpBodyRequest put(const std::string &URL) {
    return HttpBodyRequest{*this, URL, HttpMethod::Put};
  };
  [[nodiscard]] HttpBodyRequest del(const std::string &URL) {
    return HttpBodyRequest{*this, URL, HttpMethod::Delete};
  };

private:
  CURL *curl_handle = nullptr;
  // response body
  static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                               void *userdata);
  // response headers
  static size_t header_callback(char *buffer, size_t size, size_t nitems,
                                void *userdata);
  std::unordered_map<std::string, std::string> headers_;

  // main logic to perform the request
  // this is invoked by HttpRequest
  std::expected<HttpResponse, std::string> execute_get(HttpRequest &request);
  std::expected<HttpResponse, std::string> execute_head(HttpRequest &request);
  std::expected<HttpResponse, std::string>
  execute_post(HttpBodyRequest &request);
  std::expected<HttpResponse, std::string>
  execute_delete(HttpBodyRequest &request);

  const std::unordered_map<std::string, std::string> &getHeaders() const {
    return headers_;
  }
};

} // namespace s3cpp

#endif
