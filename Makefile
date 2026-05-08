.PHONY: help test test-py test-c lint build build-py build-c clean

BUILD_DIR ?= build

help:
	@echo "Targets:"
	@echo "  test        Run all tests (python + c)"
	@echo "  test-py     Run Python tests with pytest"
	@echo "  test-c      Run C tests with ctest"
	@echo "  lint        Run ruff on Python sources"
	@echo "  build       Build everything (python wheel + c library)"
	@echo "  build-py    Build the Python wheel via uv"
	@echo "  build-c     Configure and build the C library and tests"
	@echo "  clean       Remove build artefacts"

test: test-py test-c

test-py:
	uv run pytest test; status=$$?; [ $$status -eq 0 ] || [ $$status -eq 5 ]

test-c: build-c
	ctest --test-dir $(BUILD_DIR) --output-on-failure

lint:
	uv run ruff check .

build: build-py build-c

build-py:
	uv build

build-c:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) dist .pytest_cache .ruff_cache .coverage htmlcov
	find . -type d -name __pycache__ -exec rm -rf {} +
