CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
BUILD   = build

TARGETS = $(BUILD)/p1_gitstat \
          $(BUILD)/gen_gitstat \
          $(BUILD)/flt_gitstat \
          $(BUILD)/rep_gitstat

all: $(TARGETS)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%_gitstat: %_gitstat.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

p1: $(BUILD)/p1_gitstat

fifo: $(BUILD)/gen_gitstat $(BUILD)/flt_gitstat $(BUILD)/rep_gitstat

run-p1: p1
	./$(BUILD)/p1_gitstat

run-fifo: fifo
	bash scripts/run_fifo.sh

clean:
	rm -rf $(BUILD) *.o

.PHONY: all p1 fifo run-p1 run-fifo clean
