#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { PAYLOAD_BYTES = 160, DATA_PER_BLOCK = 4, PARITY_PER_BLOCK = 3 };

static uint8_t gf_exp[512];
static uint8_t gf_log[256];

static void gf_init(void) {
    uint16_t value = 1;
    for (int i = 0; i < 255; ++i) {
        gf_exp[i] = (uint8_t)value;
        gf_log[value] = (uint8_t)i;
        value <<= 1;
        if (value & 0x100) value ^= 0x11d;
    }
    for (int i = 255; i < 512; ++i) gf_exp[i] = gf_exp[i - 255];
}

static uint8_t gf_mul(uint8_t left, uint8_t right) {
    return left && right ? gf_exp[gf_log[left] + gf_log[right]] : 0;
}

static uint8_t coefficient(int parity_index, int data_index) {
    if (data_index == 0) return 1;
    return gf_exp[(parity_index + 1) * data_index];
}

int main(void) {
    int input = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in source = {0};
    source.sin_family = AF_INET;
    source.sin_port = htons(47010);
    source.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(input, (struct sockaddr *)&source, sizeof source) < 0) {
        perror("bind 47010");
        return 1;
    }

    int output = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");
    gf_init();

    uint8_t block[DATA_PER_BLOCK][PAYLOAD_BYTES];
    uint32_t block_start = 0;
    int block_count = 0;
    uint8_t input_packet[164];
    for (;;) {
        ssize_t received = recvfrom(input, input_packet, sizeof input_packet, 0, NULL, NULL);
        if (received != sizeof input_packet) continue;
        uint32_t network_seq;
        memcpy(&network_seq, input_packet, sizeof network_seq);
        uint32_t sequence = ntohl(network_seq);
        uint32_t expected_start = sequence - (sequence % DATA_PER_BLOCK);
        if (block_count == 0) block_start = expected_start;
        if (expected_start != block_start) {
            block_count = 0;
            block_start = expected_start;
        }

        uint8_t media[165];
        media[0] = 0;
        memcpy(media + 1, input_packet, sizeof input_packet);
        sendto(output, media, sizeof media, 0, (struct sockaddr *)&relay, sizeof relay);

        int data_index = (int)(sequence % DATA_PER_BLOCK);
        memcpy(block[data_index], input_packet + 4, PAYLOAD_BYTES);
        ++block_count;
        if (block_count != DATA_PER_BLOCK) continue;

        for (int parity_index = 0; parity_index < PARITY_PER_BLOCK; ++parity_index) {
            uint8_t parity[166] = {1};
            uint32_t network_start = htonl(block_start);
            memcpy(parity + 1, &network_start, sizeof network_start);
            parity[5] = (uint8_t)parity_index;
            for (int byte = 0; byte < PAYLOAD_BYTES; ++byte) {
                for (int data = 0; data < DATA_PER_BLOCK; ++data)
                    parity[6 + byte] ^= gf_mul(coefficient(parity_index, data), block[data][byte]);
            }
            sendto(output, parity, sizeof parity, 0, (struct sockaddr *)&relay, sizeof relay);
        }
        block_count = 0;
    }
}
