#pragma once

#include "duckdb.hpp"
#include "duckdb/common/arrow/arrow_wrapper.hpp"
#include <arrow/api.h>
#include <arrow/c/bridge.h>
#include <arrow-adbc/adbc.h>

namespace duckdb {

	class ArrowStreamWrapper {
	public:
		ArrowStreamWrapper();
		~ArrowStreamWrapper();

		void InitializeFromADBC(AdbcStatement* statement);
		bool GetNextBatch(std::shared_ptr<arrow::RecordBatch>& batch);
		unique_ptr<ArrowArrayWrapper> GetNextChunk();
		std::shared_ptr<arrow::Schema> GetSchema() const { return schema; }

	private:
		struct ArrowArrayStream stream;
		std::shared_ptr<arrow::RecordBatchReader> reader;
		std::shared_ptr<arrow::Schema> schema;
		bool initialized = false;
	};

} // namespace duckdb
