#ifndef HEADER_GAENARI_SUPUL_SUPUL_IMPL_STRING_TABLE_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_IMPL_STRING_TABLE_HPP

// footer included from supul.hpp
namespace supul {
namespace supul {

inline void supul_t::string_table::init(void) {
	load();
}

inline void supul_t::string_table::deinit(void) {
}

inline void supul_t::string_table::load(void) {
	// read text, and add to cache.
	supul.db->get_string_table([&strings=strings](const auto& row) -> bool {
		auto  id   = common::get_variant_int(row, "id");
		auto& text = common::get_variant_string_ref(row, "text");
		strings.add(text, id);
		return true;
	});
}

inline int supul_t::string_table::add(_in const std::string& s) {
	return strings.add(s);
}

inline const gaenari::common::string_table& supul_t::string_table::get_table(void) const {
	return strings;
}

inline void supul_t::string_table::flush(void) {
	int last_id_db		= supul.db->get_string_table_last_id();
	int last_id_cache	= strings.get_last_id();
	int db_count		= supul.db->count_string_table();

	// on empty string table.
	if (db_count == 0) last_id_db = -1;

	if (last_id_db < last_id_cache) {
		// cache string table is bigger than db.
		// insert.
		for (int i=last_id_db+1; i<=last_id_cache; i++) {
			auto& s = strings.get_string(i);
			// new record.
			supul.db->add_string_table(i, s);
		}
	}
}

} // supul
} // supul

#endif // HEADER_GAENARI_SUPUL_SUPUL_IMPL_STRING_TABLE_HPP
