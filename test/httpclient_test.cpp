#include <chrono>
#include <curl/curl.h>
#include <curl/easy.h>
#include <exception>
#include <format>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <s3cpp/httpclient.h>
#include <stdexcept>
#include <string>
#include <type_traits>

TEST(HTTP, AllStatusCodes) {
    HttpClient client {};
    for (auto i : { 200, 400, 401, 403, 404, 500, 502 }) {
        auto result = client.get(std::format("https://postman-echo.com/status/{}", i))
                                   .execute();
        ASSERT_TRUE(result.has_value());
        HttpResponse request = result.value();
        EXPECT_EQ(request.status(), i);
        EXPECT_EQ(request.is_ok(), (i >= 200 && i < 300));
        EXPECT_EQ(request.is_client_error(), (i >= 400 && i < 500));
        EXPECT_EQ(request.is_server_error(), (i >= 500 && i < 600));
    }
}

TEST(HTTP, HTTPClientCopy) {
    // duh, the compiler should not generate them
    EXPECT_FALSE(std::is_trivially_copy_constructible<HttpClient>());
    EXPECT_FALSE(std::is_trivially_copy_assignable<HttpClient>());
}

TEST(HTTP, HTTPClientMoveOwnership) {
    EXPECT_TRUE(std::is_move_constructible<HttpClient>());
    EXPECT_TRUE(std::is_move_assignable<HttpClient>());

    HttpClient client1 {};
    HttpClient client2(std::move(client1));

    const std::string URL = "https://postman-echo.com/status/200";

    // Invalidated client1 should return error
    auto result1 = client1.get(URL).execute();
    EXPECT_FALSE(result1.has_value());
    EXPECT_THAT(result1.error(), testing::HasSubstr("cURL handle is invalid"));

    auto result2 = client2.get(URL).execute();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().status(), 200);
}

TEST(HTTP, HTTPBodyNonEmpty) {
    HttpClient client {};
    auto result = client.get("https://postman-echo.com/get?foo=bar").execute();
    ASSERT_TRUE(result.has_value());
    HttpResponse request = result.value();
    EXPECT_TRUE(request.is_ok());
    EXPECT_THAT(request.body(), testing::HasSubstr("\"foo\":\"bar\""));
}

TEST(HTTP, HTTPHandleTimeout) {
    HttpClient client {};
    auto result = client.get("https://postman-echo.com/delay/10").timeout(1).execute();
    EXPECT_FALSE(result.has_value());
    EXPECT_THAT(result.error(), testing::HasSubstr("Timeout"));
}

TEST(HTTP, HTTPRequestInDifferentPhases) {
    HttpClient client {};
    // TODO(cristian): Allow for floating points on timeout?
    HttpRequest req = client.get("https://postman-echo.com/get").timeout(1);
    auto responses = std::vector<HttpResponse> {};

    auto result1 = req.execute();
    if (result1.has_value()) {
        responses.push_back(result1.value());
    }

    auto result2 = req.execute();
    if (result2.has_value()) {
        responses.push_back(result2.value());
    }

    ASSERT_EQ(responses.size(), 2);
    ASSERT_EQ(responses[0].status(), responses[1].status());
    ASSERT_EQ(responses[0].body(), responses[1].body());
    ASSERT_NE(responses[0].headers(), responses[1].headers());
}

TEST(HTTP, HTTPRequestCastTimeout) {
    HttpClient client {};

    // int (broadcasted to long)
    HttpRequest request = client.get("foo").timeout(1).timeout(2);
    EXPECT_EQ(request.getTimeout(), 2);

    // std::chrono
    request.timeout(std::chrono::hours(1));
    EXPECT_EQ(request.getTimeout(), 3600);
}

TEST(HTTP, HTTPClientDefaultHeadersOverwrittenByRequestHeaders) {
    // HttpClient allows setting headers once through a initializer list
    // this headers are then merged (and overriden) with the HttpRequest headers
    HttpClient client({
        { "Authentication", "mytoken" },
        { "User-Agent", "foo" },
    });

    // Send request with custom user-agent header, should override client header
    for (const auto& val : { "foo", "bar" }) {

        // Change the user-agent header from the request
        HttpRequest request = client.get("https://postman-echo.com/get")
                                  .timeout(10)
                                  .header("User-Agent", val);

        auto result = request.execute();
        ASSERT_TRUE(result.has_value());
        auto resp = result.value();

        // request should work + having the new user-agent header each time
        EXPECT_TRUE(resp.is_ok());
        EXPECT_THAT(resp.body(),
            testing::HasSubstr(std::format("\"user-agent\":\"{}\"", val)));
    }
}

TEST(HTTP, HTTPRemoveHeader) {
    HttpClient client(std::unordered_map<std::string, std::string>({
        { "User-Agent", "client" },
    }));

    // On libcurl passing an empty header removes it
    //     curl_slist_append(list, "Header:");
    // in which case, when merging request and client headers, if we remove a
    // request one, the client one should also be removed!

    // Removes the User-Agent header
    auto result = client.get("https://postman-echo.com/get")
                    .timeout(10)
                    .header("User-Agent", "")
                    .execute();
    ASSERT_TRUE(result.has_value());
    auto resp = result.value();
    EXPECT_THAT(resp.body(),
        testing::Not(testing::HasSubstr("\"user-agent\":\"client\"")));
}

TEST(HTTP, HTTPHead) {
    HttpClient client {};
    auto result = client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2").execute();
    ASSERT_TRUE(result.has_value());
    HttpResponse resp = result.value();
    EXPECT_TRUE(resp.body().empty());
    EXPECT_FALSE(resp.headers().empty());
}

TEST(HTTP, HTTPHeadStatusCode) {
    HttpClient client {};
    auto result = client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2").execute();
    ASSERT_TRUE(result.has_value());
    HttpResponse resp = result.value();
    EXPECT_TRUE(resp.is_ok());
    EXPECT_EQ(resp.status(), 200);
    EXPECT_TRUE(resp.body().empty());
    EXPECT_FALSE(resp.headers().empty());
}

TEST(HTTP, HTTPHeaderOrder) {
    HttpClient client {};
    auto result1 = client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2").execute();
    ASSERT_TRUE(result1.has_value());
    HttpResponse resp1 = result1.value();
    EXPECT_TRUE(resp1.body().empty());
    EXPECT_FALSE(resp1.headers().empty());

    auto result2 = client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2").execute();
    ASSERT_TRUE(result2.has_value());
    HttpResponse resp2 = result2.value();
    EXPECT_TRUE(resp2.body().empty());
    EXPECT_FALSE(resp2.headers().empty());

    // check that both headers are in the same order as well as same headers
    EXPECT_EQ(resp1.headers().size(), resp2.headers().size());
    auto it1 = resp1.headers().cbegin();
    auto it2 = resp2.headers().cbegin();
    auto end1 = resp1.headers().cend();
    auto end2 = resp2.headers().cend();
    while (it1 != end1 && it2 != end2) {
        EXPECT_EQ(it1->first, it2->first);
        it1++;
        it2++;
    }
}

TEST(HTTP, HTTPPost) {
    HttpClient client {};
    std::string data = "This is expected to be sent back as part of response body";
    HttpBodyRequest req = client.post("https://postman-echo.com/post").body(data);
    EXPECT_EQ(req.getBody(), data);
    auto result = req.execute();
    ASSERT_TRUE(result.has_value());
    HttpResponse resp = result.value();
    EXPECT_TRUE(resp.is_ok());
    EXPECT_EQ(resp.status(), 200);
    EXPECT_THAT(resp.body(), testing::HasSubstr(data));
}

// NOTE(cristian): This is done at compile time, duh, nothing to check
/*
TEST(HTTP, HTTPGetHeadCRTP) {
        // This validates our changes we did with CRTP
        // When issuing a GET/HEAD we cannot do a .body()
        // this is determined at compile time
        HttpClient client{};
        HttpRequest req =
client.head("https://postman-echo.com/get?foo0=bar1&foo2=bar2");
        // Check that we cannot do body
}
*/

// We send it as a POST because I cannot find a PUT endpoint to validate
TEST(HTTP, HTTPPut) {
    HttpClient client {};
    std::string data = "This is expected to be sent back as part of response body";
    HttpBodyRequest req = client.post("https://postman-echo.com/post").body(data);
    EXPECT_EQ(req.getBody(), data);
    auto result = req.execute();
    ASSERT_TRUE(result.has_value());
    HttpResponse resp = result.value();
    EXPECT_TRUE(resp.is_ok());
    EXPECT_EQ(resp.status(), 200);
    EXPECT_THAT(resp.body(), testing::HasSubstr(data));
}

// TEST(HTTP, HTTPDeleteQueryParamStr) {
//     HttpClient client {};
//     std::string query = "deletethis";
//     HttpBodyRequest req = client.del(std::format("https://postman-echo.com/patch?{}", query));
//     HttpResponse resp = req.execute();
//     EXPECT_TRUE(resp.is_ok());
//     EXPECT_EQ(resp.status(), 200);
//     EXPECT_THAT(resp.body(), testing::HasSubstr(query));
// }
//
// TEST(HTTP, HTTPDeleteWithBody) {
//     HttpClient client {};
//     std::string data = "This is expected to be deleted";
//     HttpBodyRequest req = client.del("https://postman-echo.com/patch").body(data);
//     EXPECT_EQ(req.getBody(), data);
//     HttpResponse resp = req.execute();
//     EXPECT_TRUE(resp.is_ok());
//     EXPECT_EQ(resp.status(), 200);
//     EXPECT_THAT(resp.body(), testing::HasSubstr(data));
// }
