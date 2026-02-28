cmake_minimum_required(VERSION 3.31.6)
project(s3cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# GoogleTest
include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/6ec14dfd8c409d05fba94e18e3a02df35b874353.zip
)
FetchContent_MakeAvailable(googletest)

# cURL
find_package(CURL REQUIRED)

# OpenSSL (SHA256)
find_package(OpenSSL REQUIRED)

add_library(s3cpplib STATIC
	src/s3cpp/httpclient.cpp
	src/s3cpp/auth.cpp
	src/s3cpp/xml.hpp
	src/s3cpp/types.h
	src/s3cpp/s3.cpp
)
target_include_directories(s3cpplib PUBLIC src)
target_link_libraries(s3cpplib PUBLIC CURL::libcurl OpenSSL::Crypto)

set_target_properties(s3cpplib PROPERTIES
	OUTPUT_NAME "s3cpp"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
add_executable(s3cpp_app main.cpp)
target_link_libraries(s3cpp_app s3cpplib)

# Testing
enable_testing()
add_executable(tests 
	test/httpclient_test.cpp
	test/auth_test.cpp
	test/xml_test.cpp
	test/s3_test.cpp
	test/aws_test.cpp
)

target_link_libraries(tests s3cpplib GTest::gtest_main GTest::gmock_main CURL::libcurl OpenSSL::Crypto)

include(GoogleTest)
gtest_discover_tests(tests)
