#include "snowflake_arrow_utils.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {

	// ArrowStreamWrapper implementation
	ArrowStreamWrapper::ArrowStreamWrapper() {
		std::memset(&stream, 0, sizeof(stream));
	}

	ArrowStreamWrapper::~ArrowStreamWrapper() {
		if (initialized && stream.release) {
			stream.release(&stream);
		}
	}

	void ArrowStreamWrapper::InitializeFromADBC(AdbcStatement* statement) {
		AdbcError error;
		std::memset(&error, 0, sizeof(error));

		AdbcStatusCode status = AdbcStatementExecuteQuery(statement, &stream, nullptr, &error);
		if (status != ADBC_STATUS_OK) {
			std::string error_msg = "Failed to execute query: ";
			if (error.message) {
				error_msg += error.message;
				if (error.release) {
					error.release(&error);
				}
			}
			throw IOException(error_msg);
		}

		// Import the stream to Arrow C++ API
		auto result = arrow::ImportRecordBatchReader(&stream);
		if (!result.ok()) {
			throw IOException("Failed to import Arrow stream: " + result.status().ToString());
		}

		reader = *result;
		schema = reader->schema();
		initialized = true;
	}

	bool ArrowStreamWrapper::GetNextBatch(std::shared_ptr<arrow::RecordBatch>& batch) {
		if (!initialized || !reader) {
			return false;
		}

		auto result = reader->Next();
		if (!result.ok()) {
			throw IOException("Failed to read next batch: " + result.status().ToString());
		}

		batch = *result;
		return batch != nullptr;
	}

	unique_ptr<ArrowArrayWrapper> ArrowStreamWrapper::GetNextChunk() {
		std::shared_ptr<arrow::RecordBatch> batch;
		if (!GetNextBatch(batch) || !batch) {
			return nullptr;
		}

		// Convert arrow::RecordBatch to ArrowArrayWrapper
		auto chunk = make_uniq<ArrowArrayWrapper>();
		auto status = arrow::ExportRecordBatch(*batch, &chunk->arrow_array);
		if (!status.ok()) {
			throw IOException("Failed to export record batch: " + status.ToString());
		}

		return chunk;
	}

} // namespace duckdb
