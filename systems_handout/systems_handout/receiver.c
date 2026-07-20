#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { PAYLOAD_BYTES = 160, DATA_PER_BLOCK = 4, PARITY_PER_BLOCK = 3, BLOCK_SLOTS = 7, GROUP_CACHE = 256 };

typedef struct {
    uint32_t start;
    uint8_t present[BLOCK_SLOTS];
    uint8_t fragment[BLOCK_SLOTS][PAYLOAD_BYTES];
} Group;

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

static uint8_t gf_inv(uint8_t value) { return gf_exp[255 - gf_log[value]]; }

static uint8_t coefficient(int parity_index, int data_index) {
    if (data_index == 0) return 1;
    return gf_exp[(parity_index + 1) * data_index];
}

static int invert(uint8_t matrix[4][4], uint8_t inverse[4][4]) {
    memset(inverse, 0, 16);
    for (int i = 0; i < 4; ++i) inverse[i][i] = 1;
    for (int column = 0; column < 4; ++column) {
        int pivot = column;
        while (pivot < 4 && matrix[pivot][column] == 0) ++pivot;
        if (pivot == 4) return 0;
        if (pivot != column) {
            for (int i = 0; i < 4; ++i) {
                uint8_t swap = matrix[column][i]; matrix[column][i] = matrix[pivot][i]; matrix[pivot][i] = swap;
                swap = inverse[column][i]; inverse[column][i] = inverse[pivot][i]; inverse[pivot][i] = swap;
            }
        }
        uint8_t scale = gf_inv(matrix[column][column]);
        for (int i = 0; i < 4; ++i) {
            matrix[column][i] = gf_mul(matrix[column][i], scale);
            inverse[column][i] = gf_mul(inverse[column][i], scale);
        }
        for (int row = 0; row < 4; ++row) {
            if (row == column) continue;
            uint8_t factor = matrix[row][column];
            for (int i = 0; i < 4; ++i) {
                matrix[row][i] ^= gf_mul(factor, matrix[column][i]);
                inverse[row][i] ^= gf_mul(factor, inverse[column][i]);
            }
        }
    }
    return 1;
}

static void deliver(int output, const struct sockaddr_in *player, uint32_t sequence, const uint8_t payload[PAYLOAD_BYTES]) {
    uint8_t packet[164];
    uint32_t network_sequence = htonl(sequence);
    memcpy(packet, &network_sequence, 4);
    memcpy(packet + 4, payload, PAYLOAD_BYTES);
    sendto(output, packet, sizeof packet, 0, (const struct sockaddr *)player, sizeof *player);
}

static void recover_and_deliver(Group *group, int output, const struct sockaddr_in *player) {
    int selected[4], count = 0;
    for (int slot = 0; slot < BLOCK_SLOTS && count < 4; ++slot)
        if (group->present[slot]) selected[count++] = slot;
    if (count < 4) return;

    uint8_t matrix[4][4], inverse[4][4];
    for (int row = 0; row < 4; ++row) {
        int slot = selected[row];
        for (int column = 0; column < 4; ++column)
            matrix[row][column] = slot < 4 ? (slot == column) : coefficient(slot - 4, column);
    }
    if (!invert(matrix, inverse)) return;
    for (int data = 0; data < 4; ++data) {
        uint8_t payload[PAYLOAD_BYTES] = {0};
        for (int row = 0; row < 4; ++row)
            for (int byte = 0; byte < PAYLOAD_BYTES; ++byte)
                payload[byte] ^= gf_mul(inverse[data][row], group->fragment[selected[row]][byte]);
        deliver(output, player, group->start + (uint32_t)data, payload);
    }
}

int main(void) {
    int input = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay_input = {0};
    relay_input.sin_family = AF_INET;
    relay_input.sin_port = htons(47002);
    relay_input.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(input, (struct sockaddr *)&relay_input, sizeof relay_input) < 0) {
        perror("bind 47002");
        return 1;
    }
    int output = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");
    gf_init();

    Group groups[GROUP_CACHE] = {0};
    uint8_t packet[512];
    for (;;) {
        ssize_t received = recvfrom(input, packet, sizeof packet, 0, NULL, NULL);
        if (received < 1) continue;
        uint32_t sequence, start;
        int slot;
        const uint8_t *payload;
        if (packet[0] == 0 && received == 165) {
            memcpy(&sequence, packet + 1, 4);
            sequence = ntohl(sequence);
            start = sequence - (sequence % DATA_PER_BLOCK);
            slot = (int)(sequence % DATA_PER_BLOCK);
            payload = packet + 5;
        } else if (packet[0] == 1 && received == 166 && packet[5] < PARITY_PER_BLOCK) {
            memcpy(&start, packet + 1, 4);
            start = ntohl(start);
            slot = DATA_PER_BLOCK + packet[5];
            payload = packet + 6;
        } else continue;
        Group *group = &groups[(start / DATA_PER_BLOCK) % GROUP_CACHE];
        if (group->start != start) {
            memset(group, 0, sizeof *group);
            group->start = start;
        }
        if (!group->present[slot]) {
            memcpy(group->fragment[slot], payload, PAYLOAD_BYTES);
            group->present[slot] = 1;
        }
        recover_and_deliver(group, output, &player);
    }
}
