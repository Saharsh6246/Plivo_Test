#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { FRAME_BYTES = 164, PAYLOAD_BYTES = 160, PARITY_BYTES = 162, CACHE_SIZE = 4096 };

typedef struct {
    uint32_t sequence;
    uint8_t present;
    uint8_t emitted;
    uint8_t payload[PAYLOAD_BYTES];
} Frame;

typedef struct {
    uint32_t sequence;
    uint8_t present;
    uint8_t value[PAYLOAD_BYTES];
} Parity;

static Frame frames[CACHE_SIZE];
static Parity parities[CACHE_SIZE];

static Frame *frame_for(uint32_t sequence) {
    Frame *frame = &frames[sequence % CACHE_SIZE];
    if (frame->sequence != sequence) {
        memset(frame, 0, sizeof *frame);
        frame->sequence = sequence;
    }
    return frame;
}

static Parity *parity_for(uint32_t sequence) {
    Parity *parity = &parities[sequence % CACHE_SIZE];
    if (parity->sequence != sequence) {
        memset(parity, 0, sizeof *parity);
        parity->sequence = sequence;
    }
    return parity;
}

static void emit_frame(int output, const struct sockaddr_in *player, Frame *frame) {
    if (!frame->present || frame->emitted) return;
    uint8_t packet[FRAME_BYTES];
    uint32_t network_sequence = htonl(frame->sequence);
    memcpy(packet, &network_sequence, 4);
    memcpy(packet + 4, frame->payload, PAYLOAD_BYTES);
    sendto(output, packet, sizeof packet, 0, (const struct sockaddr *)player, sizeof *player);
    frame->emitted = 1;
}

static int restore(Frame *target, const Frame *other, const Parity *parity) {
    if (target->present || !other->present || !parity->present) return 0;
    for (int byte = 0; byte < PAYLOAD_BYTES; ++byte)
        target->payload[byte] = other->payload[byte] ^ parity->value[byte];
    target->present = 1;
    return 1;
}

static void recover_nearby(uint32_t sequence, int output, const struct sockaddr_in *player) {
    uint32_t first = sequence > 32 ? sequence - 32 : 1;
    uint32_t last = sequence + 32;
    int changed;
    do {
        changed = 0;
        for (uint32_t current = first; current <= last; ++current) {
            Parity *parity = parity_for(current);
            if (!parity->present) continue;
            Frame *current_frame = frame_for(current);
            Frame *previous_frame = frame_for(current - 1);
            changed |= restore(current_frame, previous_frame, parity);
            changed |= restore(previous_frame, current_frame, parity);
        }
    } while (changed);
    for (uint32_t current = first; current <= last; ++current)
        emit_frame(output, player, frame_for(current));
}

int main(void) {
    for (int index = 0; index < CACHE_SIZE; ++index) {
        frames[index].sequence = UINT32_MAX;
        parities[index].sequence = UINT32_MAX;
    }

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

    uint8_t packet[FRAME_BYTES];
    for (;;) {
        ssize_t received = recvfrom(input, packet, sizeof packet, 0, NULL, NULL);
        if (received == FRAME_BYTES) {
            uint32_t network_sequence;
            memcpy(&network_sequence, packet, 4);
            uint32_t sequence = ntohl(network_sequence);
            Frame *frame = frame_for(sequence);
            if (!frame->present) {
                memcpy(frame->payload, packet + 4, PAYLOAD_BYTES);
                frame->present = 1;
            }
            emit_frame(output, &player, frame);
            recover_nearby(sequence, output, &player);
        } else if (received == PARITY_BYTES) {
            uint16_t network_sequence;
            memcpy(&network_sequence, packet, 2);
            uint32_t sequence = ntohs(network_sequence);
            Parity *parity = parity_for(sequence);
            if (!parity->present) {
                memcpy(parity->value, packet + 2, PAYLOAD_BYTES);
                parity->present = 1;
            }
            recover_nearby(sequence, output, &player);
        }
    }
}
