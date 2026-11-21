CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lm
SOURCES := $(shell find src -name '*.c' | sort)
TESTS := $(shell find tests -name 'test_*.c' | sort)
.PHONY: all test check-rules sanitize clean demo
all: build/signaldesk
build/signaldesk: $(SOURCES) $(wildcard include/*.h)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)
test:
	@mkdir -p build/tests
	@set -e; for f in $(TESTS); do n=$$(basename "$$f" .c); echo "Testing $$n"; $(CC) $(CPPFLAGS) $(CFLAGS) "$$f" $(filter-out src/main.c,$(SOURCES)) -o "build/tests/$$n" $(LDLIBS); "build/tests/$$n"; done
check-rules:
	@if grep -R -n -E 'qsort|bsearch|TODO|NOT_IMPLEMENTED|placeholder' src include; then echo 'Forbidden token found'; exit 1; else echo 'Rule scan passed'; fi
sanitize:
	@mkdir -p build/sanitize
	@set -e; for f in $(TESTS); do n=$$(basename "$$f" .c); $(CC) $(CPPFLAGS) -std=c17 -Wall -Wextra -Wpedantic -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined "$$f" $(filter-out src/main.c,$(SOURCES)) -o "build/sanitize/$$n" $(LDLIBS); "build/sanitize/$$n"; done
demo: all
	sh scripts/demo.sh
clean:
	rm -rf build

