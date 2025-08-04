PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=snowflake
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Arrow ADBC build integration
build-adbc-release:
	mkdir -p ./arrow-adbc/c/build && \
	cd ./arrow-adbc/c && \
	cmake . -DADBC_DRIVER_SNOWFLAKE=ON -DCMAKE_BUILD_TYPE=Release -B build && \
	cmake --build build --config Release && \
	mkdir -p ../../build/release && \
	find build -name "*.so" -exec cp {} ../../build/release/ \;
	$(MAKE) copy-adbc-to-extension BUILD_TYPE=release

build-adbc-debug:
	mkdir -p ./arrow-adbc/c/build-debug && \
	cd ./arrow-adbc/c && \
	cmake . -DADBC_DRIVER_SNOWFLAKE=ON -DCMAKE_BUILD_TYPE=Debug -B build-debug && \
	cmake --build build-debug --config Debug && \
	mkdir -p ../../build/debug && \
	find build-debug -name "*.so" -exec cp {} ../../build/debug/ \;
	$(MAKE) copy-adbc-to-extension BUILD_TYPE=debug

# Common step to copy ADBC driver to extension directory for RUNPATH ($ORIGIN) resolution
copy-adbc-to-extension:
	@echo "Copying ADBC driver to extension directory for $(BUILD_TYPE) build..."
	@mkdir -p build/$(BUILD_TYPE)/extension/snowflake
	@if [ -f build/$(BUILD_TYPE)/libadbc_driver_snowflake.so ]; then \
		cp build/$(BUILD_TYPE)/libadbc_driver_snowflake.so build/$(BUILD_TYPE)/extension/snowflake/; \
		echo "ADBC driver copied to extension directory"; \
	else \
		echo "Warning: ADBC driver not found in build/$(BUILD_TYPE)/"; \
	fi

# Custom targets that build ADBC before the standard targets
release-snowflake: build-adbc-release release

debug-snowflake: build-adbc-debug debug

# Clean ADBC artifacts
clean-adbc:
	rm -rf ./arrow-adbc/c/build ./arrow-adbc/c/build-debug

# Extend the clean target
clean-all: clean clean-adbc

# Make the custom targets the default
.DEFAULT_GOAL := release-snowflake

# Convenience aliases
all: release-snowflake
test: test_release