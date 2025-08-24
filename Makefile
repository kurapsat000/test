PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=snowflake
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# ADBC driver build configuration
ADBC_DIR=${PROJ_DIR}adbc
ADBC_BUILD_DIR=${PROJ_DIR}build/adbc

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Custom targets for ADBC driver
.PHONY: build_adbc release-snowflake

build_adbc:
	@echo "Building Arrow ADBC Snowflake driver..."
	mkdir -p $(ADBC_BUILD_DIR)
	cd $(ADBC_BUILD_DIR) && \
	cmake -DADBC_DRIVER_SNOWFLAKE=ON \
	      -DADBC_BUILD_SHARED=OFF \
	      -DADBC_BUILD_STATIC=ON \
	      -DCMAKE_BUILD_TYPE=Release \
	      $(ADBC_DIR)/c/driver_manager && \
	make -j$(shell nproc)
	@echo "ADBC driver built successfully"

release-snowflake: build_adbc release
	@echo "Snowflake extension built with ADBC driver"
