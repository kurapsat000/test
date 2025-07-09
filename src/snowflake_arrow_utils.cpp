#include "snowflake_arrow_utils.hpp"
#include "duckdb/common/exception.hpp"
// The following includes are no longer needed as we use DuckDB's native Arrow conversion
// #include "duckdb/common/types/date.hpp"
// #include "duckdb/common/types/timestamp.hpp"
// #include "duckdb/common/types/decimal.hpp"
// #include <arrow/compute/api.h>

namespace duckdb {

// No longer needed - DuckDB's ArrowTableFunction::PopulateArrowTableType handles type conversion
/*
LogicalType SnowflakeArrowUtils::ArrowTypeToDuckDB(const arrow::DataType& arrow_type) {
    switch (arrow_type.id()) {
        case arrow::Type::BOOL:
            return LogicalType::BOOLEAN;
        case arrow::Type::INT8:
            return LogicalType::TINYINT;
        case arrow::Type::INT16:
            return LogicalType::SMALLINT;
        case arrow::Type::INT32:
            return LogicalType::INTEGER;
        case arrow::Type::INT64:
            return LogicalType::BIGINT;
        case arrow::Type::UINT8:
            return LogicalType::UTINYINT;
        case arrow::Type::UINT16:
            return LogicalType::USMALLINT;
        case arrow::Type::UINT32:
            return LogicalType::UINTEGER;
        case arrow::Type::UINT64:
            return LogicalType::UBIGINT;
        case arrow::Type::FLOAT:
            return LogicalType::FLOAT;
        case arrow::Type::DOUBLE:
            return LogicalType::DOUBLE;
        case arrow::Type::STRING:
        case arrow::Type::LARGE_STRING:
            return LogicalType::VARCHAR;
        case arrow::Type::DATE32:
        case arrow::Type::DATE64:
            return LogicalType::DATE;
        case arrow::Type::TIMESTAMP:
            return LogicalType::TIMESTAMP;
        case arrow::Type::DECIMAL128:
        case arrow::Type::DECIMAL256: {
            auto decimal_type = std::static_pointer_cast<arrow::DecimalType>(arrow_type.GetSharedPtr());
            return LogicalType::DECIMAL(decimal_type->precision(), decimal_type->scale());
        }
        case arrow::Type::LIST:
        case arrow::Type::LARGE_LIST: {
            auto list_type = std::static_pointer_cast<arrow::ListType>(arrow_type.GetSharedPtr());
            auto child_type = ArrowTypeToDuckDB(*list_type->value_type());
            return LogicalType::LIST(child_type);
        }
        case arrow::Type::STRUCT: {
            auto struct_type = std::static_pointer_cast<arrow::StructType>(arrow_type.GetSharedPtr());
            child_list_t<LogicalType> children;
            for (int i = 0; i < struct_type->num_fields(); i++) {
                auto field = struct_type->field(i);
                auto child_type = ArrowTypeToDuckDB(*field->type());
                children.push_back(make_pair(field->name(), child_type));
            }
            return LogicalType::STRUCT(children);
        }
        default:
            throw NotImplementedException("Unsupported Arrow type: %s", arrow_type.ToString().c_str());
    }
}
*/

// No longer needed - DuckDB's ArrowTableFunction::PopulateArrowTableType handles type conversion
/*
vector<LogicalType> SnowflakeArrowUtils::GetDuckDBTypes(const arrow::Schema& schema) {
    vector<LogicalType> types;
    for (int i = 0; i < schema.num_fields(); i++) {
        types.push_back(ArrowTypeToDuckDB(*schema.field(i)->type()));
    }
    return types;
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertArrowChunkToDuckDB(const arrow::RecordBatch& batch,
                                                    DataChunk& chunk,
                                                    idx_t start_row) {
    idx_t num_rows = batch.num_rows() - start_row;
    if (num_rows > STANDARD_VECTOR_SIZE) {
        num_rows = STANDARD_VECTOR_SIZE;
    }
    
    chunk.SetCardinality(num_rows);
    
    for (idx_t col_idx = 0; col_idx < chunk.ColumnCount(); col_idx++) {
        auto& vector = chunk.data[col_idx];
        auto& array = *batch.column(col_idx);
        ConvertArrowArrayToDuckDB(array, vector, start_row, num_rows);
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertArrowArrayToDuckDB(const arrow::Array& array, 
                                                    Vector& vector, 
                                                    idx_t offset,
                                                    idx_t size) {
    switch (array.type()->id()) {
        case arrow::Type::BOOL:
            ConvertBoolArray(array, vector, offset, size);
            break;
        case arrow::Type::INT8:
            ConvertNumericArray<int8_t>(array, vector, offset, size);
            break;
        case arrow::Type::INT16:
            ConvertNumericArray<int16_t>(array, vector, offset, size);
            break;
        case arrow::Type::INT32:
            ConvertNumericArray<int32_t>(array, vector, offset, size);
            break;
        case arrow::Type::INT64:
            ConvertNumericArray<int64_t>(array, vector, offset, size);
            break;
        case arrow::Type::UINT8:
            ConvertNumericArray<uint8_t>(array, vector, offset, size);
            break;
        case arrow::Type::UINT16:
            ConvertNumericArray<uint16_t>(array, vector, offset, size);
            break;
        case arrow::Type::UINT32:
            ConvertNumericArray<uint32_t>(array, vector, offset, size);
            break;
        case arrow::Type::UINT64:
            ConvertNumericArray<uint64_t>(array, vector, offset, size);
            break;
        case arrow::Type::FLOAT:
            ConvertNumericArray<float>(array, vector, offset, size);
            break;
        case arrow::Type::DOUBLE:
            ConvertNumericArray<double>(array, vector, offset, size);
            break;
        case arrow::Type::STRING:
        case arrow::Type::LARGE_STRING:
            ConvertStringArray(array, vector, offset, size);
            break;
        case arrow::Type::DATE32:
        case arrow::Type::DATE64:
            ConvertDateArray(array, vector, offset, size);
            break;
        case arrow::Type::TIMESTAMP:
            ConvertTimestampArray(array, vector, offset, size);
            break;
        case arrow::Type::DECIMAL128:
        case arrow::Type::DECIMAL256:
            ConvertDecimalArray(array, vector, offset, size);
            break;
        case arrow::Type::LIST:
        case arrow::Type::LARGE_LIST:
            ConvertListArray(array, vector, offset, size);
            break;
        case arrow::Type::STRUCT:
            ConvertStructArray(array, vector, offset, size);
            break;
        default:
            throw NotImplementedException("Unsupported Arrow type for conversion: %s", 
                                        array.type()->ToString().c_str());
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
template<typename T>
void SnowflakeArrowUtils::ConvertNumericArray(const arrow::Array& array, 
                                              Vector& vector, 
                                              idx_t offset, 
                                              idx_t size) {
    auto& typed_array = static_cast<const arrow::NumericArray<typename arrow::CTypeTraits<T>::ArrowType>&>(array);
    auto data = FlatVector::GetData<T>(vector);
    auto& validity = FlatVector::Validity(vector);
    
    for (idx_t i = 0; i < size; i++) {
        if (typed_array.IsNull(offset + i)) {
            validity.SetInvalid(i);
        } else {
            data[i] = typed_array.Value(offset + i);
        }
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertBoolArray(const arrow::Array& array, 
                                           Vector& vector, 
                                           idx_t offset, 
                                           idx_t size) {
    auto& bool_array = static_cast<const arrow::BooleanArray&>(array);
    auto data = FlatVector::GetData<bool>(vector);
    auto& validity = FlatVector::Validity(vector);
    
    for (idx_t i = 0; i < size; i++) {
        if (bool_array.IsNull(offset + i)) {
            validity.SetInvalid(i);
        } else {
            data[i] = bool_array.Value(offset + i);
        }
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertStringArray(const arrow::Array& array, 
                                             Vector& vector, 
                                             idx_t offset, 
                                             idx_t size) {
    auto& string_array = static_cast<const arrow::StringArray&>(array);
    auto& validity = FlatVector::Validity(vector);
    
    for (idx_t i = 0; i < size; i++) {
        if (string_array.IsNull(offset + i)) {
            validity.SetInvalid(i);
        } else {
            auto str_view = string_array.GetView(offset + i);
            FlatVector::GetData<string_t>(vector)[i] = StringVector::AddString(vector, 
                                                                               str_view.data(), 
                                                                               str_view.length());
        }
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertDateArray(const arrow::Array& array, 
                                          Vector& vector, 
                                          idx_t offset, 
                                          idx_t size) {
    auto data = FlatVector::GetData<date_t>(vector);
    auto& validity = FlatVector::Validity(vector);
    
    if (array.type()->id() == arrow::Type::DATE32) {
        auto& date_array = static_cast<const arrow::Date32Array&>(array);
        for (idx_t i = 0; i < size; i++) {
            if (date_array.IsNull(offset + i)) {
                validity.SetInvalid(i);
            } else {
                // Arrow DATE32 is days since epoch
                data[i] = date_t(date_array.Value(offset + i));
            }
        }
    } else {
        auto& date_array = static_cast<const arrow::Date64Array&>(array);
        for (idx_t i = 0; i < size; i++) {
            if (date_array.IsNull(offset + i)) {
                validity.SetInvalid(i);
            } else {
                // Arrow DATE64 is milliseconds since epoch
                data[i] = date_t(date_array.Value(offset + i) / 86400000);
            }
        }
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertTimestampArray(const arrow::Array& array, 
                                               Vector& vector, 
                                               idx_t offset, 
                                               idx_t size) {
    auto& ts_array = static_cast<const arrow::TimestampArray&>(array);
    auto data = FlatVector::GetData<timestamp_t>(vector);
    auto& validity = FlatVector::Validity(vector);
    
    auto ts_type = std::static_pointer_cast<arrow::TimestampType>(array.type());
    
    for (idx_t i = 0; i < size; i++) {
        if (ts_array.IsNull(offset + i)) {
            validity.SetInvalid(i);
        } else {
            int64_t value = ts_array.Value(offset + i);
            
            // Convert to microseconds based on Arrow timestamp unit
            switch (ts_type->unit()) {
                case arrow::TimeUnit::SECOND:
                    value *= 1000000;
                    break;
                case arrow::TimeUnit::MILLI:
                    value *= 1000;
                    break;
                case arrow::TimeUnit::MICRO:
                    // Already in microseconds
                    break;
                case arrow::TimeUnit::NANO:
                    value /= 1000;
                    break;
            }
            
            data[i] = Timestamp::FromEpochMicroSeconds(value);
        }
    }
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertListArray(const arrow::Array& array, 
                                          Vector& vector, 
                                          idx_t offset, 
                                          idx_t size) {
    // TODO: Implement list conversion
    throw NotImplementedException("List array conversion not yet implemented");
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertStructArray(const arrow::Array& array, 
                                            Vector& vector, 
                                            idx_t offset, 
                                            idx_t size) {
    // TODO: Implement struct conversion
    throw NotImplementedException("Struct array conversion not yet implemented");
}
*/

// No longer needed - using DuckDB's native ArrowTableFunction::ArrowToDuckDB
/*
void SnowflakeArrowUtils::ConvertDecimalArray(const arrow::Array& array, 
                                             Vector& vector, 
                                             idx_t offset, 
                                             idx_t size) {
    // TODO: Implement decimal conversion
    throw NotImplementedException("Decimal array conversion not yet implemented");
}
*/

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