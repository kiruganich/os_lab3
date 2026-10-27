# release build because we measure speed
BUILD  ?= build
GAUSS  := $(BUILD)/gauss

all:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release > /dev/null
	cmake --build $(BUILD) -j

run: all
	./$(GAUSS) -t 4 < examples/system4.txt

test: all
	bash ./tests/smoke_test.sh $(abspath $(GAUSS))

bench: all
	bash ./tests/bench.sh $(abspath $(GAUSS))

clean:
	rm -rf $(BUILD)

.PHONY: all run test bench clean
