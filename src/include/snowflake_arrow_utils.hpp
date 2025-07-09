#pragma once

#include "duckdb.hpp"
#include "duckdb/common/arrow/arrow_wrapper.hpp"
#include <arrow/api.h>
#include <arrow/c/bridge.h>
#include <arrow-adbc/adbc.h>

namespace duckdb {

class SnowflakeArrowUtils {
public:
    // No longer needed - DuckDB's ArrowTableFunction::PopulateArrowTableType handles type conversion
    // static LogicalType ArrowTypeToDuckDB(const arrow::DataType& arrow_type);
    
    // No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
    /*
    static void ConvertArrowArrayToDuckDB(const arrow::Array& array, 
                                         Vector& vector, 
                                         idx_t offset,
                                         idx_t size);
    
    static void ConvertArrowChunkToDuckDB(const arrow::RecordBatch& batch,
                                         DataChunk& chunk,
                                         idx_t start_row = 0);
    */
    
    // No longer needed - DuckDB's ArrowTableFunction::PopulateArrowTableType handles type conversion
    // static vector<LogicalType> GetDuckDBTypes(const arrow::Schema& schema);
    
private:
    // No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
    /*
    template<typename T>
    static void ConvertNumericArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    
    static void ConvertStringArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertBoolArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertDateArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertTimestampArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertListArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertStructArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    static void ConvertDecimalArray(const arrow::Array& array, Vector& vector, idx_t offset, idx_t size);
    */
};

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