#ifndef HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_ENGINE_HPP
#define HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_ENGINE_HPP

// footer included from sqlite.hpp

namespace supul {
namespace db {
namespace sqlite {

inline void sqlite_t::create_table_if_not_existed(_in const db::schema& schema) {
	int rc;
	std::string sql;

	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;

	auto& tables = schema.get_tables();
	for (const auto& table: tables) {
		sql = "CREATE TABLE IF NOT EXISTS ${" + table.name + "} (";
		for (const auto& it: table.fields) {
			const auto& name = it.first;
			const auto& type = it.second;
			sql += common::quote(name) + ' ';
			if (name == "id") {
				// in sqlite, only INTEGER is allowed for autoincrement.
				sql += "INTEGER PRIMARY KEY AUTOINCREMENT";
			} else {
				if      (type == type::field_type::INTEGER)  sql += "INTEGER";
				else if (type == type::field_type::TEXT)     sql += "TEXT";
				else if (type == type::field_type::TEXT_ID)  sql += "INTEGER";
				else if (type == type::field_type::REAL)     sql += "REAL";
				else if (type == type::field_type::BIGINT)   sql += "BIGINT";
				else if (type == type::field_type::SMALLINT) sql += "SMALLINT";
				else if (type == type::field_type::TINYINT)  sql += "TINYINT";
				else THROW_SUPUL_ERROR("invalid field type.");
			}
			sql += ',';
		}
		sql.pop_back();
		sql += ");";
		sql = schema.get_sql(sql);
		rc = sqlite3_exec(this->db, sql.c_str(), nullptr, nullptr, nullptr);
		if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to create_table_if_not_existed", error_desc(rc));
	}
}

inline void sqlite_t::create_index_if_not_existed(_in const db::schema& schema) {
	int rc;
	std::string sql;
	std::string name;

	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;

	auto& tables = schema.get_tables();
	for (const auto& table: tables) {
		if (table.indexes.empty()) continue;
		for (const auto& index: table.indexes) {
			name = "idx_" + schema.get_real_table_name(table.name) + '_' + index.column_name;
			sql = "CREATE INDEX IF NOT EXISTS " + name + " ON ${" + table.name + "} (" + common::quote(index.column_name) + ")";
			sql = schema.get_sql(sql);
			rc = sqlite3_exec(this->db, sql.c_str(), nullptr, nullptr, nullptr);
			if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to create_index_if_not_existed", error_desc(rc));
		}
	}
}

inline std::vector<gaenari::dataset::usecols> sqlite_t::get_usecols(_in const type::fields& fields) const {
	std::vector<gaenari::dataset::usecols> usecols;
	std::vector<std::string> int32, int64, string, dbl, string_table;

	for (auto& it: fields) {
		const auto& name = it.first;
		const auto& type = it.second;
		switch (type) {
			case type::field_type::INTEGER:
			case type::field_type::SMALLINT:
			case type::field_type::TINYINT:
				int32.emplace_back(name);
				break;
			case type::field_type::BIGINT:
				int64.emplace_back(name);
				break;
			case type::field_type::TEXT_ID:
				string_table.emplace_back(name);
				break;
			case type::field_type::REAL:
				dbl.emplace_back(name);
				break;
			case type::field_type::TEXT:
				string.emplace_back(name);
				break;
			default:
				THROW_SUPUL_INTERNAL_ERROR0;
		}
	}

	// int32, int64, string, dbl, string_table.
	if (not int32.empty())			usecols.emplace_back(gaenari::dataset::usecols::names(int32,		gaenari::dataset::data_type_t::data_type_int));
	if (not int64.empty())			usecols.emplace_back(gaenari::dataset::usecols::names(int64,		gaenari::dataset::data_type_t::data_type_int64));
	if (not string.empty())			usecols.emplace_back(gaenari::dataset::usecols::names(string,		gaenari::dataset::data_type_t::data_type_string));
	if (not dbl.empty())			usecols.emplace_back(gaenari::dataset::usecols::names(dbl,			gaenari::dataset::data_type_t::data_type_double));
	if (not string_table.empty())	usecols.emplace_back(gaenari::dataset::usecols::names(string_table,	gaenari::dataset::data_type_t::data_type_string_table));

	// error on empty.
	if (usecols.empty()) THROW_SUPUL_INTERNAL_ERROR0;

	return usecols;
}

// execute the query with callback.
// assign the `?` part of stmt to params in order.
// remark) the number or type of `?` will not be checked in params.
//         invalid params may return incorrect results.
// ex) select ... from ... where id=? and cars=?
//     params : {3, 2} (id=3, cars=2)
// ex) execute(type::stmt::select..., {1, 3}, [](const auto& row) -> bool {
//         ...
//         return true; // continue
//     });
inline void sqlite_t::execute(_in stmt stmt_type, _in const type::vector_variant& params, _in callback_query cb) {
	int rc = 0;
	int pos = 1;
	bool error = false;
	type::map_variant row;
	std::string name;

	if (not cb) THROW_SUPUL_INTERNAL_ERROR0;

	// find stmt.
	auto& stmt = find_stmt(stmt_type);

	// bind stmt.
	for (const auto& param: params) {
		auto index = param.index();
		if (index == 0) {			// monostate.
			THROW_SUPUL_INTERNAL_ERROR0;
		} else if (index == 1) {	// int64_t.
			rc = sqlite3_bind_int64(stmt.stmt, pos, std::get<1>(param));
		} else if (index == 2) {	// double.
			rc = sqlite3_bind_double(stmt.stmt, pos, std::get<2>(param));
		} else if (index == 3) {	// string.
			rc = sqlite3_bind_text(stmt.stmt, pos, std::get<3>(param).c_str(), -1, SQLITE_STATIC);
		} else THROW_SUPUL_INTERNAL_ERROR0;
		if (rc != SQLITE_OK) {
			THROW_SUPUL_DB_ERROR("fail to execute", error_desc(rc));
		}
		pos++;
	}

	// execute.
	// guaranteed to call reset(...).
	try {
		for (;;) {
			// execute.
			rc = sqlite3_step(stmt.stmt);
			if (rc == SQLITE_ROW) {
				// queried a row.
				// set row data.
				auto count = sqlite3_column_count(stmt.stmt);
				for (pos=0; pos<count; pos++) {
					auto _name = sqlite3_column_name(stmt.stmt, pos);
					if (not _name) { error = true; THROW_SUPUL_INTERNAL_ERROR0; }
					name = std::string(_name);
					auto find = stmt.fields.find(name);
					if (find == stmt.fields.end()) { error = true; THROW_SUPUL_INTERNAL_ERROR0; }
					auto _column_type = sqlite3_column_type(stmt.stmt, pos);
					if (_column_type == SQLITE_NULL) {
						row[name] = {};	// monostate.
						continue;
					}
					auto& type = find->second;
					switch (type) {
						case type::field_type::INTEGER:
						case type::field_type::BIGINT:
						case type::field_type::SMALLINT:
						case type::field_type::TINYINT:
						case type::field_type::TEXT_ID:
							row[name] = static_cast<int64_t>(sqlite3_column_int64(stmt.stmt, pos));
							break;
						case type::field_type::REAL:
							row[name] = sqlite3_column_double(stmt.stmt, pos);
							break;
						case type::field_type::TEXT:
							row[name] = (const char*)(sqlite3_column_text(stmt.stmt, pos));
							break;
						default:
							error = true;
							THROW_SUPUL_INTERNAL_ERROR0;
					}
				}
				// call callback.
				if (not (cb)(row)) break;
			} else if (rc == SQLITE_DONE) {
				// query completed.
				break;
			} else if (rc == SQLITE_BUSY) {
				error = true; THROW_SUPUL_DB_ERROR("fail to sqlite3_step(SQLITE_BUSY).", error_desc(rc));
			} else {
				error = true;
				THROW_SUPUL_DB_ERROR("fail to sqlite3_step.", error_desc(rc));
			}
		}
		sqlite3_reset(stmt.stmt);
	} catch(...) {
		exceptions::catch_all();
		error = true;
		sqlite3_reset(stmt.stmt);
	}

	if (error) THROW_SUPUL_INTERNAL_ERROR0;
}

// after executing the query, the result is returned as a dataframe.
// assign the `?` part of stmt to params in order.
// remark) the number or type of `?` will not be checked in params.
//         invalid params may return incorrect results.
// ex) select ... from ... where id=? and cars=?
//     params : {3, 2} (id=3, cars=2)
inline gaenari::dataset::dataframe sqlite_t::execute(_in stmt stmt_type, _in const type::vector_variant& params) {
	int rc = 0;
	gaenari::dataset::dataframe df;

	// find stmt.
	auto& stmt = find_stmt(stmt_type);

	// get dataframe usecols for query result field.
	auto usecols = get_usecols(stmt.fields);

	// bind stmt.
	int pos = 1;
	for (const auto& param: params) {
		size_t index = param.index();
		if (index == 0) {			// monostate.
			THROW_SUPUL_INTERNAL_ERROR0;
		} else if (index == 1) {	// int64_t.
			rc = sqlite3_bind_int64(stmt.stmt, pos, std::get<1>(param));
		} else if (index == 2) {	// double.
			rc = sqlite3_bind_double(stmt.stmt, pos, std::get<2>(param));
		} else if (index == 3) {	// string.
			rc = sqlite3_bind_text(stmt.stmt, pos, std::get<3>(param).c_str(), -1, SQLITE_STATIC);
		} else THROW_SUPUL_INTERNAL_ERROR0;
		if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to execute", error_desc(rc));
	}

	// set repository context.
	repository::context context = {stmt_type, params, this};

	// read from repository.
	df.read<repository>({}, usecols, &context);

	// return dataframe.
	return df;
}

// execute the query and return the first queried row.
// this is used when the query is not in the form of a table.
// - insert ...
// - drop ...
// if there is one result of the query, the first row is returned.
// - select count(*) ...
// - insert ... returning id
// when error_on_multiple_rows is true, if there are multiple rows like a normal `select`, an error occurs.
// 
// remark) the number or type of `?` will not be checked in params.
//         invalid params may return incorrect results.
// ex) select ... from ... where id=? and cars=?
//     params : {3, 2} (id=3, cars=2)
inline auto sqlite_t::execute(_in stmt stmt_type, _in const type::vector_variant& params, _option_in bool error_on_multiple_row) -> type::map_variant {
	int row_count = 0;
	type::map_variant row;

	// execution with callback.
	execute(stmt_type, params, [&row_count, &row, error_on_multiple_row](const auto& _row) -> bool {
		row_count++;
		if (row_count == 1) {
			// first row.
			row = _row;
		}
		if (error_on_multiple_row and (row_count >= 2)) return false;
		return true;
	});

	if (error_on_multiple_row and (row_count >= 2)) THROW_SUPUL_ERROR("multiple row error.");
	return row;
}

// column based results like parquet.
inline auto sqlite_t::execute_column_based(_in stmt stmt_type, _in const type::vector_variant& params) -> std::unordered_map<std::string, std::vector<type::value_variant>> {
	std::unordered_map<std::string, std::vector<type::value_variant>> ret;

	// execution.
	execute(stmt_type, params, [&ret](const auto& _row) -> bool {
		for (const auto& it: _row) {
			const auto& name  = it.first;
			const auto& value = it.second;
			ret[name].emplace_back(value);
		}
		return true;
	});

	return ret;
}

// returns a vector<map> similar to a dataframe.
// used when the number of rows is small.
inline void sqlite_t::execute(_in stmt stmt_type, _in const type::vector_variant& params, _out std::vector<type::map_variant>& results, _option_in size_t max_row_counts) {
	size_t cur_row_index = 0;

	// clear output.
	results.clear();

	// execution with callback.
	execute(stmt_type, params, [&results, &cur_row_index, max_row_counts](const auto& _row) -> bool {
		cur_row_index++;
		if (cur_row_index > max_row_counts) THROW_SUPUL_ERROR("too much vector<map> row item counts.");
		results.emplace_back(_row);
		return true;
	});
}

// execute the query and get a column of data by name and return it as a vector.
// it is much simpler than a dataframe except that it takes only one column.
// the return vector type supports only int64_t, int and string.
template <typename T>
inline auto sqlite_t::execute(_in stmt stmt_type, _in const type::vector_variant& params, _in const std::string& name) -> std::vector<T> {
	std::vector<T> ret;

	// T type checking.
	if      constexpr (std::is_same<T, int64_t>::value) {}
	else if constexpr (std::is_same<T, int>::value) {}
	else if constexpr (std::is_same<T, double>::value) {}
	else if constexpr (std::is_same<T, std::string>::value) {}
	else static_assert(gaenari::type::dependent_false_v<T>, "internal compile error!");

	// execution with callback.
	execute(stmt_type, params, [&ret, &name](const auto& row) -> bool {
		auto find = row.find(name);
		if (find == row.end()) THROW_SUPUL_ERROR("name is not found, " + name);
		auto& v = find->second;
		auto index = v.index();

		if constexpr (std::is_same<std::string, T>::value) {
			if (index == 3) {			// string.
				ret.emplace_back(std::get<3>(v));
			} else THROW_SUPUL_INTERNAL_ERROR0;
		} else {
			if (index == 1) {			// int64_t.
				ret.push_back(static_cast<T>(std::get<1>(v)));
			} else if (index == 2) {	// double.
				ret.push_back(static_cast<T>(std::get<2>(v)));
			} else THROW_SUPUL_INTERNAL_ERROR0;
		}

		return true;
	});

	return ret;
}

inline sqlite3_stmt* sqlite_t::get_stmt(_in const std::string& sql) {
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare(this->db, sql.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		gaenari::logger::error("sql error: {0}", {sql});
		THROW_SUPUL_DB_ERROR("fail to sqlite3_prepare.", error_desc(rc));
	}
	return stmt;
}

inline stmt_info& sqlite_t::find_stmt(_in stmt stmt_type) {
	auto find = stmt_pool.find(stmt_type);
	if (find == stmt_pool.end()) THROW_SUPUL_INTERNAL_ERROR0;
	return find->second;
}

inline void sqlite_t::clear_stmt_pool(void) {
	for (auto& it: stmt_pool) {
		sqlite3_reset(it.second.stmt);
		sqlite3_finalize(it.second.stmt);
	}
	stmt_pool.clear();
}

inline std::string sqlite_t::error_desc(_in const std::optional<int> error_code) const {
	std::string ret;

	// get error msg.
	auto msg = sqlite3_errmsg(this->db);
	if (not msg) msg = "sqlite error.";
	ret = msg;

	if (error_code.has_value()) {
		ret += ' ';
		auto& code = error_code.value();
		if (code == SQLITE_OK) return ret += "(SQLITE_OK)"; if (code == SQLITE_ERROR) return ret += "(SQLITE_ERROR)"; if (code == SQLITE_INTERNAL) return ret += "(SQLITE_INTERNAL)";
		if (code == SQLITE_PERM) return ret += "(SQLITE_PERM)"; if (code == SQLITE_ABORT) return ret += "(SQLITE_ABORT)"; if (code == SQLITE_BUSY) return ret += "(SQLITE_BUSY)";
		if (code == SQLITE_LOCKED) return ret += "(SQLITE_LOCKED)"; if (code == SQLITE_NOMEM) return ret += "(SQLITE_NOMEM)"; if (code == SQLITE_READONLY) return ret += "(SQLITE_READONLY)";
		if (code == SQLITE_INTERRUPT) return ret += "(SQLITE_INTERRUPT)"; if (code == SQLITE_IOERR) return ret += "(SQLITE_IOERR)"; if (code == SQLITE_CORRUPT) return ret += "(SQLITE_CORRUPT)";
		if (code == SQLITE_NOTFOUND) return ret += "(SQLITE_NOTFOUND)"; if (code == SQLITE_FULL) return ret += "(SQLITE_FULL)"; if (code == SQLITE_CANTOPEN) return ret += "(SQLITE_CANTOPEN)";
		if (code == SQLITE_PROTOCOL) return ret += "(SQLITE_PROTOCOL)"; if (code == SQLITE_EMPTY) return ret += "(SQLITE_EMPTY)"; if (code == SQLITE_SCHEMA) return ret += "(SQLITE_SCHEMA)";
		if (code == SQLITE_TOOBIG) return ret += "(SQLITE_TOOBIG)"; if (code == SQLITE_CONSTRAINT) return ret += "(SQLITE_CONSTRAINT)"; if (code == SQLITE_MISMATCH) return ret += "(SQLITE_MISMATCH)";
		if (code == SQLITE_MISUSE) return ret += "(SQLITE_MISUSE)"; if (code == SQLITE_NOLFS) return ret += "(SQLITE_NOLFS)"; if (code == SQLITE_AUTH) return ret += "(SQLITE_AUTH)";
		if (code == SQLITE_FORMAT) return ret += "(SQLITE_FORMAT)"; if (code == SQLITE_RANGE) return ret += "(SQLITE_RANGE)"; if (code == SQLITE_NOTADB) return ret += "(SQLITE_NOTADB)";
		if (code == SQLITE_NOTICE) return ret += "(SQLITE_NOTICE)"; if (code == SQLITE_WARNING) return ret += "(SQLITE_WARNING)"; if (code == SQLITE_ROW) return ret += "(SQLITE_ROW)";
		if (code == SQLITE_DONE) return ret += "(SQLITE_DONE)";
		return ret += "(code=" + std::to_string(code) + ")";
	}

	return ret;
}

} // sqlite
} // db
} // supul

#endif // HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_ENGINE_HPP
