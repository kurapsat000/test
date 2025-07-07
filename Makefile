PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=snowflake
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# IntelliSense targets for development
intellisense: ${EXTENSION_CONFIG_STEP}
	mkdir -p ./build/clangd && \
	cmake  -DEXTENSION_STATIC_BUILD=1 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DDUCKDB_EXTENSION_CONFIGS='/home/praveen/code/duckdb-snowflake/extension_config.cmake' -DENABLE_UNITTEST_CPP_TESTS=FALSE   -DCMAKE_BUILD_TYPE=Release -S "./duckdb/" -B build/release && \
	cp ./build/release/compile_commands.json ./build/clangd/ && \
	echo "IntelliSense setup complete! compile_commands.json copied to build/clangd/"


clangd: ${EXTENSION_CONFIG_STEP}
	cmake -DCMAKE_BUILD_TYPE=Debug -DEXTENSION_CONFIG_BUILD=TRUE -DVCPKG_BUILD=1 -B build/clangd .