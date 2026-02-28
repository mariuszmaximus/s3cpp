#include <gtest/gtest.h>
#include <s3cpp/xml.hpp>

using namespace s3cpp;

TEST(XML, XMLBasicTag) {
    // <Bucket>Name</Bucket>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Bucket>Name</Bucket>");
}

TEST(XML, XMLBasicTagWithXSD) {
    // <Bucket>Name</Bucket>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Bucket>Name</Bucket>");
}

TEST(XML, XMLNestedBasicTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // </Session>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Session><Bucket>Name</Bucket></Session>");
    EXPECT_EQ(XMLValues.size(), 1);
    EXPECT_EQ(XMLValues[0].tag, "Session.Bucket");
    EXPECT_EQ(XMLValues[0].value, "Name");
}

TEST(XML, XMLNestedNestedBasicTag) {
    // <Nesting>
    //    <Session>
    //       <Bucket>Name</Bucket>
    //    </Session>
    // </Nesting>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Nesting><Session><Bucket>Name</Bucket></Session></Nesting>");
    EXPECT_EQ(XMLValues.size(), 1);
    EXPECT_EQ(XMLValues.at(0).tag, "Nesting.Session.Bucket");
    EXPECT_EQ(XMLValues.at(0).value, "Name");
}

TEST(XML, XMLInvalidClosingTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // </Invalid>
    auto parser = XMLParser();
    EXPECT_ANY_THROW(parser.parse("<Session><Bucket>Name</Bucket></Invalid>"));
}

TEST(XML, XMLInvalidIncompleteTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // <Invalid>
    auto parser = XMLParser();
    EXPECT_ANY_THROW(parser.parse("<Session><Bucket>Name</Bucket><Invalid>"));
}

TEST(XML, XMLAWSListNoBuckets) {
    // <?xml version="1.0" encoding="UTF-8"?>
    // <ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Owner>
    //     <ID>02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4</ID>
    //     <DisplayName>minio</DisplayName>
    //   </Owner>
    //   <Buckets/>
    // </ListAllMyBucketsResult>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<?xml version="1.0" encoding="UTF-8"?>
		<ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Owner><ID>02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4</ID><DisplayName>minio</DisplayName></Owner><Buckets></Buckets></ListAllMyBucketsResult>)");
    EXPECT_EQ(XMLValues.size(), 2);
    EXPECT_EQ(XMLValues[0].tag, "ListAllMyBucketsResult.Owner.ID");
    EXPECT_EQ(XMLValues[0].value, "02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4");
    EXPECT_EQ(XMLValues[1].tag, "ListAllMyBucketsResult.Owner.DisplayName");
    EXPECT_EQ(XMLValues[1].value, "minio");
}

TEST(XML, XMLAWSListBucket) {
    // <?xml version="1.0" encoding="UTF-8"?>
    // <ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Owner>
    //     <ID>02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4</ID>
    //     <DisplayName>minio</DisplayName>
    //   </Owner>
    //   <Buckets>
    //     <Bucket>
    //       <Name>cristian-vault</Name>
    //       <CreationDate>2025-12-07T14:32:30.240Z</CreationDate>
    //     </Bucket>
    //   </Buckets>
    // </ListAllMyBucketsResult>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<?xml version="1.0" encoding="UTF-8"?>
		<ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Owner><ID>02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4</ID><DisplayName>minio</DisplayName></Owner><Buckets><Bucket><Name>cristian-vault</Name><CreationDate>2025-12-07T14:32:30.240Z</CreationDate></Bucket></Buckets></ListAllMyBucketsResult>)");
    EXPECT_EQ(XMLValues.size(), 4);
    EXPECT_EQ(XMLValues[0].tag, "ListAllMyBucketsResult.Owner.ID");
    EXPECT_EQ(XMLValues[0].value, "02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4");
    EXPECT_EQ(XMLValues[1].tag, "ListAllMyBucketsResult.Owner.DisplayName");
    EXPECT_EQ(XMLValues[1].value, "minio");
    EXPECT_EQ(XMLValues[2].tag, "ListAllMyBucketsResult.Buckets.Bucket.Name");
    EXPECT_EQ(XMLValues[2].value, "cristian-vault");
    EXPECT_EQ(XMLValues[3].tag, "ListAllMyBucketsResult.Buckets.Bucket.CreationDate");
    EXPECT_EQ(XMLValues[3].value, "2025-12-07T14:32:30.240Z");
}

TEST(XML, XMLHandleDecimalEntity) {
    // <?xml version="1.0" encoding="UTF-8"?>
    // <ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Contents>
    //     <ETag>&#34;This ETag has quotes!&#34;</ETag>
    //   </Contents>
    // </ListBucketResult>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<?xml version="1.0" encoding="UTF-8"?><ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Contents><ETag>&#34;This ETag has quotes!&#34;</ETag></Contents></ListBucketResult>)");
    EXPECT_EQ(XMLValues.size(), 1);
    EXPECT_EQ(XMLValues[0].tag, "ListBucketResult.Contents.ETag");
    EXPECT_EQ(XMLValues[0].value, "\"This ETag has quotes!\"");
}

TEST(XML, XMLHandleHexEntity) {
    // <?xml version="1.0" encoding="UTF-8"?>
    // <ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Contents>
    //     <ETag>&#x22;This ETag has quotes!&#x22;</ETag>
    //   </Contents>
    // </ListBucketResult>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<?xml version="1.0" encoding="UTF-8"?><ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Contents><ETag>&#x22;This ETag has quotes!&#x22;</ETag></Contents></ListBucketResult>)");
    EXPECT_EQ(XMLValues.size(), 1);
    EXPECT_EQ(XMLValues[0].tag, "ListBucketResult.Contents.ETag");
    EXPECT_EQ(XMLValues[0].value, "\"This ETag has quotes!\"");
}

TEST(XML, XMLParseError) {
    // <?xml version="1.0" encoding="UTF-8"?>
    // <Error>
    //   <Code>NoSuchKey</Code>
    //   <Message>The resource you requested does not exist</Message>
    //   <Resource>/mybucket/myfoto.jpg</Resource>
    //   <RequestId>4442587FB7D0A2F9</RequestId>
    // </Error>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<?xml version="1.0" encoding="UTF-8"?><Error><Code>NoSuchKey</Code><Message>The resource you requested does not exist</Message><Resource>/mybucket/myfoto.jpg</Resource><RequestId>4442587FB7D0A2F9</RequestId></Error>)");
    EXPECT_EQ(XMLValues.size(), 4);
    EXPECT_EQ(XMLValues[0], (XMLNode { .tag = "Error.Code", .value = "NoSuchKey" }));
    EXPECT_EQ(XMLValues[1], (XMLNode { .tag = "Error.Message", .value = "The resource you requested does not exist" }));
    EXPECT_EQ(XMLValues[2], (XMLNode { .tag = "Error.Resource", .value = "/mybucket/myfoto.jpg" }));
    EXPECT_EQ(XMLValues[3], (XMLNode { .tag = "Error.RequestId", .value = "4442587FB7D0A2F9" }));
}

TEST(XML, XMLParseBucketNotEmpty) {
	// <Error>
	//    <Code>BucketNotEmpty</Code>
	//    <Message>The bucket you tried to delete is not empty</Message>
	//    <BucketName>bucket123</BucketName>
	//    <Resource>/bucket123</Resource>
	//    <RequestId>188D1887F999813F</RequestId>
	//    <HostId>dd9025bab4ad464b049177c95eb6ebf374d3b3fd1af9251148b658df7ac2e3e8</HostId>
	// </Error>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<Error><Code>BucketNotEmpty</Code><Message>The bucket you tried to delete is not empty</Message><BucketName>bucket123</BucketName><Resource>/bucket123</Resource><RequestId>188D1887F999813F</RequestId><HostId>dd9025bab4ad464b049177c95eb6ebf374d3b3fd1af9251148b658df7ac2e3e8</HostId></Error>)");
    EXPECT_EQ(XMLValues.size(), 6);
    EXPECT_EQ(XMLValues[0], (XMLNode { .tag = "Error.Code", .value = "BucketNotEmpty" }));
    EXPECT_EQ(XMLValues[1], (XMLNode { .tag = "Error.Message", .value = "The bucket you tried to delete is not empty" }));
    EXPECT_EQ(XMLValues[3], (XMLNode { .tag = "Error.Resource", .value = "/bucket123" }));
}

TEST(XML, XMLTrailingSpaces) {
    // <Bucket>          Name</Bucket>
    auto parser = XMLParser();
    auto XMLValues = parser.parse(R"(<Bucket>          Name</Bucket>)");
    EXPECT_EQ(XMLValues.size(), 1);
    EXPECT_EQ(XMLValues[0], (XMLNode { .tag = "Bucket", .value = "Name" }));
}
