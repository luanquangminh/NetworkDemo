#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../src/common/protocol.h"

void test_packet_create_and_free(void) {
    printf("Testing packet_create and packet_free...\n");

    Packet* pkt = packet_create(CMD_LOGIN_REQ, "test payload", 12);
    assert(pkt != NULL);
    assert(pkt->magic[0] == MAGIC_BYTE_1);
    assert(pkt->magic[1] == MAGIC_BYTE_2);
    assert(pkt->command == CMD_LOGIN_REQ);
    assert(pkt->data_length == 12);
    assert(strcmp(pkt->payload, "test payload") == 0);

    packet_free(pkt);
    printf("PASSED\n");
}

void test_encode_decode_roundtrip(void) {
    printf("Testing encode/decode roundtrip...\n");

    const char* test_json = "{\"username\":\"admin\",\"password\":\"test\"}";
    Packet* original = packet_create(CMD_LOGIN_REQ, test_json, strlen(test_json));

    uint8_t buffer[1024];
    int encoded_size = packet_encode(original, buffer, sizeof(buffer));
    assert(encoded_size == (int)(HEADER_SIZE + strlen(test_json)));

    Packet decoded = {0};
    int result = packet_decode(buffer, encoded_size, &decoded);
    assert(result == 0);
    assert(decoded.command == original->command);
    assert(decoded.data_length == original->data_length);
    assert(strcmp(decoded.payload, original->payload) == 0);

    packet_free(original);
    free(decoded.payload);
    printf("PASSED\n");
}

void test_invalid_magic(void) {
    printf("Testing invalid magic bytes rejection...\n");

    uint8_t buffer[] = {0x00, 0x00, CMD_LOGIN_REQ, 0, 0, 0, 0};
    Packet pkt = {0};

    int result = packet_decode(buffer, sizeof(buffer), &pkt);
    assert(result == -2);  // Invalid magic

    printf("PASSED\n");
}

void test_empty_payload(void) {
    printf("Testing empty payload...\n");

    Packet* pkt = packet_create(CMD_LOGIN_REQ, NULL, 0);
    assert(pkt != NULL);
    assert(pkt->data_length == 0);
    assert(pkt->payload == NULL);

    uint8_t buffer[HEADER_SIZE];
    int encoded_size = packet_encode(pkt, buffer, sizeof(buffer));
    assert(encoded_size == HEADER_SIZE);

    Packet decoded = {0};
    int result = packet_decode(buffer, encoded_size, &decoded);
    assert(result == 0);
    assert(decoded.data_length == 0);
    assert(decoded.payload == NULL);

    packet_free(pkt);
    printf("PASSED\n");
}

void test_buffer_too_small(void) {
    printf("Testing buffer too small error...\n");

    Packet* pkt = packet_create(CMD_LOGIN_REQ, "test", 4);
    uint8_t small_buffer[5];  // Too small

    int result = packet_encode(pkt, small_buffer, sizeof(small_buffer));
    assert(result == -1);  // Should fail

    packet_free(pkt);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Protocol Unit Tests ===\n\n");

    test_packet_create_and_free();
    test_encode_decode_roundtrip();
    test_invalid_magic();
    test_empty_payload();
    test_buffer_too_small();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
