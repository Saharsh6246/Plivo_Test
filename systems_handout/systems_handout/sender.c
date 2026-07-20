#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { FRAME_BYTES = 164, PAYLOAD_BYTES = 160, PARITY_PERIOD = 25 };

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

    uint8_t previous[PAYLOAD_BYTES];
    int have_previous = 0;
    uint8_t frame[FRAME_BYTES];
    for (;;) {
        ssize_t received = recvfrom(input, frame, sizeof frame, 0, NULL, NULL);
        if (received != FRAME_BYTES) continue;

        uint32_t network_sequence;
        memcpy(&network_sequence, frame, sizeof network_sequence);
        uint32_t sequence = ntohl(network_sequence);
        sendto(output, frame, sizeof frame, 0, (struct sockaddr *)&relay, sizeof relay);

        if (have_previous && sequence % PARITY_PERIOD != 0) {
            uint8_t parity[PAYLOAD_BYTES + 2];
            uint16_t sequence16 = htons((uint16_t)sequence);
            memcpy(parity, &sequence16, sizeof sequence16);
            for (int byte = 0; byte < PAYLOAD_BYTES; ++byte)
                parity[2 + byte] = frame[4 + byte] ^ previous[byte];
            sendto(output, parity, sizeof parity, 0, (struct sockaddr *)&relay, sizeof relay);
        }
        memcpy(previous, frame + 4, PAYLOAD_BYTES);
        have_previous = 1;
    }
}
