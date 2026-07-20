CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

SUBMISSION_DIR := systems_handout/systems_handout

all: sender receiver

sender: $(SUBMISSION_DIR)/sender.c
	$(CC) $(CFLAGS) -o $@ $<

receiver: $(SUBMISSION_DIR)/receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f sender receiver
