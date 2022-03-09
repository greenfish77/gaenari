#ifndef HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_REPOSITORY_HPP
#define HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_REPOSITORY_HPP

namespace supul {
namespace db {
namespace sqlite {

// define repository to convert database query result into dataframe.
class repository : public gaenari::dataset::repository_base {
public:
	virtual bool open(_in const std::map<std::string,gaenari::type::variant>& options, _option_in void* repository_ctx, _out std::vector<std::string>& column_names);
	virtual size_t count(void);
	virtual void read(_in callback_read cb);
	virtual void close(void);
public:
	// context.
	struct context {
		stmt stmt_type = stmt::unknown;
		type::vector_variant params;
		sqlite_t* sqlite = nullptr;
	}* ctx = nullptr;
};

// implements.
inline bool repository::open(_in const std::map<std::string,gaenari::type::variant>& options, _option_in void* repository_ctx, _out std::vector<std::string>& column_names) {
	if (not repository_ctx) THROW_SUPUL_INTERNAL_ERROR0;

	// set context.
	ctx = static_cast<context*>(repository_ctx);

	// find stmt.
	if (not ctx->sqlite) THROW_SUPUL_INTERNAL_ERROR0;
	auto& stmt = ctx->sqlite->find_stmt(ctx->stmt_type);

	// set column names.
	column_names.clear();
	for (const auto& it: stmt.fields) {
		const auto& name = it.first;
		column_names.push_back(name);
	}

	return true;
}

inline size_t repository::count(void) {
	// auto growth mode.
	return std::numeric_limits<size_t>::max();
}

inline void repository::read(_in callback_read cb) {
	int rc = 0;
	int pos = 1;
	int column_count = 0;
	bool error = false;
	std::vector<callback_column_type> row;
	std::vector<std::string> string_pool;

	if (not ctx) THROW_SUPUL_INTERNAL_ERROR0;

	// find stmt.
	if (not ctx->sqlite) THROW_SUPUL_INTERNAL_ERROR0;
	auto& stmt = ctx->sqlite->find_stmt(ctx->stmt_type);

	// bind stmt.
	for (const auto& param: ctx->params) {
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
			THROW_SUPUL_ERROR("fail to sqlite3_bind_*.");
		}
		pos++;
	}

	// get column count, and resize row.
	column_count = static_cast<int>(stmt.fields.size());
	row.resize(column_count);
	string_pool.resize(column_count);

	// execute.
	// guaranteed to call reset(...).
	try {
		for (;;) {
			// execute.
			rc = sqlite3_step(stmt.stmt);
			if (rc == SQLITE_ROW) {
				//	queried a row.

				// set row data.
				pos = 0;
				int count = sqlite3_column_count(stmt.stmt);
				if (count != column_count) { error = true; THROW_SUPUL_INTERNAL_ERROR0; }
				for (const auto& it: stmt.fields) {
					const auto& type = it.second;
					switch (type) {
						case type::field_type::INTEGER:
						case type::field_type::BIGINT:
						case type::field_type::SMALLINT:
						case type::field_type::TINYINT:
						case type::field_type::TEXT_ID:
							row[pos] = static_cast<int64_t>(sqlite3_column_int64(stmt.stmt, pos));
							break;
						case type::field_type::REAL:
							row[pos] = sqlite3_column_double(stmt.stmt, pos);
							break;
						case type::field_type::TEXT:
							string_pool[pos] = (const char*)sqlite3_column_text(stmt.stmt, pos);
							row[pos] = string_pool[pos];
							break;
						default:
							error = true;
							THROW_SUPUL_INTERNAL_ERROR0;
					}
					pos++;
				}
				// call callback.
				if (not (cb)(row)) break;
			} else if (rc == SQLITE_DONE) {
				// query comleted.
				break;
			} else if (rc == SQLITE_BUSY) {
				error = true; THROW_SUPUL_ERROR("fail to sqlite3_step, SQLITE_BUSY.");
			} else {
				error = true; THROW_SUPUL_ERROR("fail to sqlite3_step, " + std::to_string(rc));
			}
		}
		sqlite3_reset(stmt.stmt);
	} catch(...) {
		sqlite3_reset(stmt.stmt);
	}

	if (error) THROW_SUPUL_INTERNAL_ERROR0;

	return;
}

inline void repository::close(void) {
	// do nothing...
}

} // sqlite
} // db
} // gaenari

#endif // HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_REPOSITORY_HPP
