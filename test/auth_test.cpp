#include <gtest/gtest.h>
#include <openssl/sha.h>
#include <s3cpp/auth.h>
#include <s3cpp/httpclient.h>

using namespace s3cpp;

TEST(AUTH, SHA256HexDigest) {
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");
    EXPECT_EQ(signer.hex(signer.sha256("github.com/ggcr/s3cpp")),
        "bc088c51b33c2730707dbb528d1d0bfafc59ba56c8c9aa3b8e0dc0c13e3d9b2b");
}

TEST(AUTH, HMACSHA256HexDigest) {
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");
    std::string key = "super-secret-key";
    EXPECT_EQ(signer.hex(signer.HMAC_SHA256(
                  reinterpret_cast<const unsigned char*>(key.c_str()),
                  key.size(),
                  "github.com/ggcr/s3cpp")),
        "558084957fb05bb4786ad6791bfbee71e67a11fea964e5dac6bac6b2f749b339");
}

TEST(AUTH, ChainedHMACSHA256HexDigest) {
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");

    // AWS SigV4 derives the key for the signature using nested HMAC_SHA256
    // This test tries to assert that HMAC(HMAC(k, v), v) works as expected
    // The hardcoded hashes are taken from: https://emn178.github.io/online-tools/sha256.html

    std::string v = "github.com/ggcr/s3cpp";

    // HMAC(k, v)
    std::string key1 = "super-secret-key";
    const unsigned char* hash1 = signer.HMAC_SHA256(reinterpret_cast<const unsigned char*>(key1.c_str()), key1.size(), v);
    EXPECT_EQ(signer.hex(hash1), "558084957fb05bb4786ad6791bfbee71e67a11fea964e5dac6bac6b2f749b339");

    // HMAC(HMAC(k, v), v)
    const unsigned char* hex2 = signer.HMAC_SHA256(hash1, SHA256_DIGEST_LENGTH, v);
    EXPECT_EQ(signer.hex(hex2), "d5a2b747dcb6b25cc4da081eedc15edef2d217d8497c67987ed9167d412d898c");
}

TEST(AUTH, CannonicalGETRequest) {
    // create signer & http client
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");
    HttpClient client {};

    // prepare request
    const std::string host = "s3.amazonaws.com";
    const std::string URI = "/amzn-s3-demo-bucket/myphoto.jpg";
    const std::string URL = std::format("http://{}{}", host, URI);
    const std::string timestamp = signer.getTimestamp();
    const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    HttpRequest req = client.get(URL)
                          .header("Host", host)
                          .header("X-Amz-Date", timestamp)
                          .header("X-Amz-Content-Sha256", empty_payload_hash);

    const std::string expected_canonical = std::format("GET\n"
                                                       "/amzn-s3-demo-bucket/myphoto.jpg\n"
                                                       "\n"
                                                       "host:{}\n"
                                                       "x-amz-content-sha256:{}\n"
                                                       "x-amz-date:{}\n"
                                                       "\n"
                                                       "host;x-amz-content-sha256;x-amz-date\n"
                                                       "{}",
        host, empty_payload_hash, timestamp, empty_payload_hash);

    EXPECT_EQ(signer.createCannonicalRequest(req, empty_payload_hash), expected_canonical);
    signer.sign(req);
    EXPECT_TRUE(req.getHeaders().contains("Authorization"));
}

TEST(AUTH, CannonicalGETRequestWithoutPayload) {
    // create signer & http client
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");
    HttpClient client {};

    // prepare request
    const std::string host = "s3.amazonaws.com";
    const std::string URI = "/amzn-s3-demo-bucket/myphoto.jpg";
    const std::string URL = std::format("http://{}{}", host, URI);
    const std::string timestamp = signer.getTimestamp();
    const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    HttpRequest req = client.get(URL)
                          .header("Host", host)
                          .header("X-Amz-Date", timestamp)
                          .header("X-Amz-Content-Sha256", empty_payload_hash);

    const std::string expected_canonical = std::format("GET\n"
                                                       "/amzn-s3-demo-bucket/myphoto.jpg\n"
                                                       "\n"
                                                       "host:{}\n"
                                                       "x-amz-content-sha256:{}\n"
                                                       "x-amz-date:{}\n"
                                                       "\n"
                                                       "host;x-amz-content-sha256;x-amz-date\n"
                                                       "{}",
        host, empty_payload_hash, timestamp, empty_payload_hash);

    EXPECT_EQ(signer.createCannonicalRequest(req), expected_canonical);
    signer.sign(req);
    EXPECT_TRUE(req.getHeaders().contains("Authorization"));
}

TEST(AUTH, MinIOBasicRequest) {
    // create signer & http client
    auto signer = AWSSigV4Signer("minio_access", "minio_secret");
    HttpClient client {};

    // prepare request
    const std::string empty_payload_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    HttpRequest req = client.get("http://127.0.0.1:9000/")
                          .header("Host", "127.0.0.1")
                          .header("X-Amz-Content-Sha256", empty_payload_hash);
    signer.sign(req);

    auto result = req.execute();
    if (!result.has_value()) {
        // Our error in the GitHub CI will be "Couldn't connect to server"
        const std::string emsg = result.error();
        // CI has a different libcurl version xd
        if (emsg == "libcurl error: Could not connect to server" || emsg == "libcurl error: Couldn't connect to server") {
            GTEST_SKIP_("Skipping MinIOBasicRequest: Server not up");
        }
        FAIL() << "Request failed: " << emsg;
    }
    HttpResponse resp = result.value();
    EXPECT_EQ(resp.status(), 200);
}

