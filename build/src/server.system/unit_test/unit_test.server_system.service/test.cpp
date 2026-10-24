#include "pch.h"

namespace CGDK
{

// ============================================================================
// Test: host_instance_local serialization
// ============================================================================
class HostInstanceLocalTest : public ::testing::Test
{
protected:
	OBJ<server_system::host::instance_local> host_instance;
	buffer buf;
};

TEST_F(HostInstanceLocalTest, instantiate)
{
	// Just verify the instance can be created
	EXPECT_TRUE(true);
}

TEST_F(HostInstanceLocalTest, serialize_out_basic)
{
	// Setup
	host_instance.info.setting.iid = 100;

	buf = alloc_shared_buffer(65535);

	// Serialize out
	host_instance.serialize_out(buf);
	EXPECT_GT(buf.size(), 0);
}

TEST_F(HostInstanceLocalTest, process_size_of)
{
	// Setup
	host_instance.info.setting.iid = 42;

	// Check size is calculated
	std::size_t size = get_size_of(host_instance);
	EXPECT_GT(size, 0);

	// Serialize and verify
	auto test_buf = alloc_shared_buffer(size);

	host_instance.serialize_out(test_buf);
	EXPECT_EQ(test_buf.size(), size);
}

// ============================================================================
// Test: service_instance_local serialization
// ============================================================================
class ServiceInstanceLocalTest : public ::testing::Test
{
protected:
	OBJ<server_system::service::instance_local> service_instance;
	buffer buf;
};

TEST_F(ServiceInstanceLocalTest, instantiate)
{
	// Just verify the instance can be created
	EXPECT_TRUE(true);
}

TEST_F(ServiceInstanceLocalTest, serialize_out_basic)
{
	// Setup
	service_instance.info.setting.iid = 200;
	service_instance.host_instance_id = 50;

	buf = alloc_shared_buffer(get_size_of(service_instance));

	// Serialize out
	service_instance.serialize_out(buf);
	EXPECT_GT(buf.size(), 0);
}

TEST_F(ServiceInstanceLocalTest, process_size_of)
{
	// Setup
	service_instance.info.setting.iid = 99;
	service_instance.host_instance_id = 77;

	// Check size
	std::size_t size = get_size_of(service_instance);
	EXPECT_GT(size, 0);

	// Serialize and verify
	auto test_buf = alloc_shared_buffer(size);
	service_instance.serialize_out(test_buf);
	EXPECT_EQ(test_buf.size(), size);
}

TEST_F(ServiceInstanceLocalTest, serialize_roundtrip)
{
	// Setup
	service_instance.info.setting.iid = 333;
	service_instance.host_instance_id = 111;
	service_instance.instance_key.key = 67890;

	// Serialize out
	buf = alloc_shared_buffer(get_size_of(service_instance));
	service_instance.serialize_out(buf);

	// Deserialize in
	buffer_view view(buf.data(), buf.size());
	OBJ<server_system::service::instance_local> service_instance2;
	service_instance2.serialize_in(view);

	// Verify
	EXPECT_EQ(service_instance2.info.setting.iid, 333);
	EXPECT_EQ(service_instance2.host_instance_id, 111);
	EXPECT_EQ(service_instance2.instance_key.key, 67890);
}

// ============================================================================
// Test: host_instance_local serialization roundtrip
// ============================================================================
TEST(HostInstanceSerializationTest, roundtrip_consistency)
{
	OBJ<server_system::host::instance_local> host1, host2;
	buffer buf;

	// Setup
	host1.info.setting.iid = 555;
	host1.instance_key.key = 12345;

	// Serialize
	buf = alloc_shared_buffer(get_size_of(host1));
	host1.serialize_out(buf);

	// Deserialize
	host2.serialize_in(buf);

	// Verify
	EXPECT_EQ(host2.info.setting.iid, 555);
	EXPECT_EQ(host2.instance_key.key, 12345);
}

} // namespace CGDK
