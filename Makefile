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

build-adbc-debug:
	mkdir -p ./arrow-adbc/c/build-debug && \
	cd ./arrow-adbc/c && \
	cmake . -DADBC_DRIVER_SNOWFLAKE=ON -DCMAKE_BUILD_TYPE=Debug -B build-debug && \
	cmake --build build-debug --config Debug && \
	mkdir -p ../../build/debug && \
	find build-debug -name "*.so" -exec cp {} ../../build/debug/ \;

# Override the standard targets to include ADBC build
release: build-adbc-release ${EXTENSION_CONFIG_STEP}
	mkdir -p build/release
	cmake $(GENERATOR) $(BUILD_FLAGS) $(EXT_RELEASE_FLAGS) $(VCPKG_MANIFEST_FLAGS) -DCMAKE_BUILD_TYPE=Release -S $(DUCKDB_SRCDIR) -B build/release
	cmake --build build/release --config Release

debug: build-adbc-debug ${EXTENSION_CONFIG_STEP}
	mkdir -p build/debug
	cmake $(GENERATOR) $(BUILD_FLAGS) $(EXT_DEBUG_FLAGS) $(VCPKG_MANIFEST_FLAGS) -DCMAKE_BUILD_TYPE=Debug -S $(DUCKDB_SRCDIR) -B build/debug
	cmake --build build/debug --config Debug

# Override clean to include ADBC cleanup
clean:
	rm -rf build
	rm -rf testext
	rm -rf ./arrow-adbc/c/build ./arrow-adbc/c/build-debug
	make clean -C $(DUCKDB_SRCDIR)