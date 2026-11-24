COMPOSE ?= docker compose
PORT ?= /dev/ttyACM0

.PHONY: help build test fs fs-flash web-dev flash monitor shell lint clean cad size

help:
	@echo "i_psb targets:"
	@echo "  make build    - compile ESP32-S3 firmware (Docker/PlatformIO)"
	@echo "  make test     - run native unit tests"
	@echo "  make fs       - copy web/ to firmware/data/www and build LittleFS image"
	@echo "  make fs-flash - upload the LittleFS image over USB"
	@echo "  make web-dev  - serve the SPA with mock API on http://localhost:8080"
	@echo "  make flash    - flash over USB (PORT=/dev/ttyACM0 by default)"
	@echo "  make monitor  - serial monitor"
	@echo "  make shell    - PlatformIO shell in the container"
	@echo "  make lint     - clang-format check"
	@echo "  make size     - firmware size report"
	@echo "  make cad      - render case.scad (requires OpenSCAD)"
	@echo "  make clean    - remove build artifacts"

build:
	$(COMPOSE) run --rm firmware

test:
	$(COMPOSE) run --rm native-tests

fs:
	./tools/package_web.sh
	$(COMPOSE) run --rm firmware pio run -t buildfs

fs-flash:
	./tools/uploadfs.sh $(PORT)

web-dev:
	$(COMPOSE) up web-dev

flash:
	./tools/flash.sh $(PORT)

monitor:
	./tools/monitor.sh $(PORT)

shell:
	$(COMPOSE) run --rm firmware-shell

lint:
	./tools/lint.sh

size:
	$(COMPOSE) run --rm firmware pio run -t size

cad:
	@mkdir -p build
	docker run --rm -v "$(PWD)":/workspace -w /workspace/cad \
		openscad/openscad:latest openscad -o /workspace/build/case.stl case.scad

clean:
	rm -rf firmware/.pio build firmware/data/www
