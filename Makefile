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
	@echo "Building Arrow ADBC Snowflake driver (placeholder)..."
	./scripts/build_adbc.sh

build_adbc_go:
	@echo "ADBC Go driver is built as part of build_adbc target"

release-snowflake: build_adbc release
	@echo "Snowflake extension built with ADBC driver"

# CI-friendly build that doesn't require ADBC
release-ci: release
	@echo "Snowflake extension built for CI (without ADBC)"
