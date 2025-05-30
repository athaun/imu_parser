#include <gtest/gtest.h>
#include "../src/parser.h"
#include <vector>
#include <cstring>

namespace IMUParser {
    uint32_t from_network_byte_order(uint32_t data);

    size_t find_packet_signature(const char* buffer, const size_t buffer_size, const size_t search_offset);

    uint32_t parse_u32(const std::vector<char>& read_buffer, size_t offset);

    float parse_float(const std::vector<char>& read_buffer, size_t offset);

    void parse_packets(std::vector<char>& read_buffer, std::vector<Packet>& packets);
}

using namespace IMUParser;

static constexpr uint32_t PACKET_SIGNATURE = 0x7FF01CAF;
static constexpr uint32_t PACKET_SIGNATURE_NET_ORDERED = 0xAF1CF07F; 
static constexpr size_t PACKET_SIZE = 20;

TEST(ParserTest, FromNetworkByteOrderWorks) {
    uint32_t net = 0x12345678; // Example value in network byte order (big-endian)
    uint32_t expected = 0x78563412; // Expected value in little-endian byte order
    EXPECT_EQ(from_network_byte_order(net), expected);
}

TEST(ParserTest, ParseU32Works) {
    // Prepare a 4-byte chunk in network byte order (big-endian)
    std::vector<char> buffer = { 0x78, 0x56, 0x34, 0x12 };
    uint32_t result = parse_u32(buffer, 0);

    // Check if the parsed value is in little-endian byte order
    EXPECT_EQ(result, from_network_byte_order(0x12345678));
}

TEST(ParserTest, ParseFloatWorks) {
    const float control_float = 3.14159f;

    // Prepare a float in network byte order (big-endian)
    uint32_t net_float;
    std::memcpy(&net_float, &control_float, sizeof(float));
    net_float = from_network_byte_order(net_float);

    // Create a buffer with the network byte order representation of the float
    std::vector<char> buffer(4);
    std::memcpy(buffer.data(), &net_float, 4);

    // Parse the float from the buffer
    float result = parse_float(buffer, 0);

    // Check if the parsed float is close (=) to the original float
    EXPECT_NEAR(result, control_float, 1e-5);
}

TEST(ParserTest, FindPacketSignatureFindsCorrectLocation) {
    // Create buffer with signature at specific location
    std::vector<char> buffer(20, 0x00);
    
    // Place signature at offset 5
    std::memcpy(buffer.data() + 5, &PACKET_SIGNATURE_NET_ORDERED, sizeof(uint32_t));
    
    size_t result = find_packet_signature(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(result, 5);
}

TEST(ParserTest, FindPacketSignatureReturnsMinusOneWhenNotFound) {
    std::vector<char> buffer(20, 0x42); // Fill with non-signature data
    
    size_t result = find_packet_signature(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(result, static_cast<size_t>(-1));
}

TEST(ParserTest, FindPacketSignatureHandlesBufferBoundary) {
    // Buffer too small to contain full signature at the end
    std::vector<char> buffer(6, 0x00);

    // Try to place signature at position where it would exceed buffer
    std::memcpy(buffer.data() + 3, &PACKET_SIGNATURE_NET_ORDERED, 3); // Only partial signature
    
    size_t result = find_packet_signature(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(result, static_cast<size_t>(-1));
}

TEST(ParserTest, ParsePacketsNoSignature) {
    std::vector<char> read_buffer(20, 0x42); // Fill with non-signature data
    std::vector<Packet> packets;
    
    parse_packets(read_buffer, packets);
    
    EXPECT_EQ(packets.size(), 0);
}

TEST(ParserTest, ParsePacket) {
    std::vector<char> read_buffer(20);
    std::vector<Packet> packets;
    
    // Create a valid packet: signature + count + 3 floats
    uint32_t count = 42;
    uint32_t net_count = from_network_byte_order(count);
    
    auto to_network_order_float = [](float value) -> uint32_t {
        uint32_t net_value;
        std::memcpy(&net_value, &value, sizeof(float));
        return from_network_byte_order(net_value);
    };

    float x_rate = 1.5f, y_rate = 2.5f, z_rate = 3.5f;
    uint32_t net_x = to_network_order_float(x_rate);
    uint32_t net_y = to_network_order_float(y_rate);
    uint32_t net_z = to_network_order_float(z_rate);
    
    // Build packet in buffer
    std::memcpy(read_buffer.data() + 0, &PACKET_SIGNATURE_NET_ORDERED, 4);
    std::memcpy(read_buffer.data() + 4, &net_count, 4);
    std::memcpy(read_buffer.data() + 8, &net_x, 4);
    std::memcpy(read_buffer.data() + 12, &net_y, 4);
    std::memcpy(read_buffer.data() + 16, &net_z, 4);
    
    parse_packets(read_buffer, packets);
    
    ASSERT_EQ(packets.size(), 1);
    EXPECT_EQ(packets[0].packet_count, count);
    EXPECT_NEAR(packets[0].X_rate_rdps, x_rate, 1e-5);
    EXPECT_NEAR(packets[0].Y_rate_rdps, y_rate, 1e-5);
    EXPECT_NEAR(packets[0].Z_rate_rdps, z_rate, 1e-5);
}