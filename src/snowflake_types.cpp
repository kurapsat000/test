#include "snowflake_types.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/exception/conversion_exception.hpp"

namespace duckdb {
	namespace snowflake {
		LogicalType SnowflakeTypeToLogicalType(const std::string& snowflake_type_str) {
			string normalized_type = StringUtil::Upper(snowflake_type_str);
			normalized_type = StringUtil::Replace(normalized_type, " ", "");

			auto paren_pos = normalized_type.find('(');
			string base_type = normalized_type.substr(0, paren_pos);

			if (base_type == "INTEGER" || base_type == "INT") {
				return LogicalType::INTEGER;
			}

			if (base_type == "VARCHAR" || base_type == "STRING") {
				return LogicalType::VARCHAR;
			}

			if (base_type == "BOOLEAN") {
				return LogicalType::BOOLEAN;
			}

			if (base_type == "NUMBER") {
				if (paren_pos == std::string::npos) {
					// TODO create user setting to specify the behavior here. DOUBLE is relatively safe but loses precision for large decimals, and could possibly be losing out on performance.
					return LogicalType::DOUBLE;
				}

				auto close_paren_pos = normalized_type.find(')');
				if (close_paren_pos == std::string::npos) {
					throw InvalidInputException("Expected closing ')' for NUMBER type: " + snowflake_type_str);
				}

				auto params = normalized_type.substr(paren_pos + 1, close_paren_pos - paren_pos - 1);

				auto comma_pos = params.find(',');
				int temp_precision = 0;
				int temp_scale = 0;

				if (comma_pos == std::string::npos) {
					try {
						temp_precision = std::stoi(params);
					}
					catch (const std::exception& e) {
						throw ConversionException("Invalid precision '%s' in type: %s",
							params, snowflake_type_str);
					}
				} else {
					std::string precision_str = params.substr(0, comma_pos);
					std::string scale_str = params.substr(comma_pos + 1);

					try {
						temp_precision = std::stoi(precision_str);
					}
					catch (const std::exception& e) {
						throw ConversionException("Invalid precision '%s' in type: %s",
							precision_str, snowflake_type_str);
					}

					try {
						temp_scale = std::stoi(scale_str);
					}
					catch (const std::exception& e) {
						throw ConversionException("Invalid scale '%s' in type: %s",
							scale_str, snowflake_type_str);
					}
				}

				if (temp_precision < 1 || temp_precision > 38) {
					throw ConversionException("NUMBER precision %d out of range (1-38)", temp_precision);
				}
				if (temp_scale < 0 || temp_scale > temp_precision) {
					throw ConversionException("NUMBER scale %d invalid (must be 0-%d)", temp_scale, temp_precision);
				}

				uint8_t precision = static_cast<uint8_t>(temp_precision);
				uint8_t scale = static_cast<uint8_t>(temp_scale);

				return ConvertNumber(precision, scale);
			}

			// TODO: Add more type mappings (TIMESTAMP_NTZ, TIMESTAMP_TZ, etc.)

			return LogicalType::VARCHAR; // fallback type
		}

		LogicalType ConvertNumber(uint8_t precision, uint8_t scale) {
			if (scale == 0) {  // Integer
				if (precision <= 2) return LogicalType::TINYINT;
				if (precision <= 4) return LogicalType::SMALLINT;
				if (precision <= 9) return LogicalType::INTEGER;
				if (precision <= 18) return LogicalType::BIGINT;
			} else {  // Has decimals
				// TODO introduce extension setting option to force using DuckDB DECIMAL for true precision (but slower computation and bigger size in memory)
				if (precision <= 7) {
					return LogicalType::FLOAT;
				}
				if (precision <= 15) {
					return LogicalType::DOUBLE;
				}
			}

			return LogicalType::DECIMAL(precision, scale);
		}
	} // namespace snowflake
} // namespace duckdb
