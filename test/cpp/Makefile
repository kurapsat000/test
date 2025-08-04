CXX = g++
CXXFLAGS = -std=c++17 -g -I../../src/include -I../../duckdb/src/include -I../../build/debug/vcpkg_installed/x64-linux/include
LDFLAGS = -L../../build/debug/extension/snowflake -L../../duckdb/build/debug/src -Wl,-rpath,../../build/debug/extension/snowflake -Wl,-rpath,../../duckdb/build/debug/src
LIBS = -lsnowflake_loadable_extension -lduckdb -lpthread -ldl -lstdc++fs

test_client: test_client_methods.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

run: test_client
	./test_client

clean:
	rm -f test_client

.PHONY: run clean