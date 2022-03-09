#ifndef HEADER_GAENARI_GAENARI_COMMON_STRING_TABLE_HPP
#define HEADER_GAENARI_GAENARI_COMMON_STRING_TABLE_HPP

namespace gaenari {
namespace common {

class string_table {
public:
	inline string_table()  = default;
	inline ~string_table() = default;

	inline void clear(void) {
		read_only = false;
		referencing = false;
		strings = _strings;
		ids = _ids;
	}

	// add string, and return it's id.
	// if string exists, the old id is returned.
	inline int add(_in const std::string& s) {
		if (read_only) THROW_GAENARI_ERROR("it cannot be added in read-only.");
		int count = static_cast<int>(ids.size());
		auto [it, inserted] = strings.insert({s, count});
		if (inserted) {
			ids.emplace_back(&it->first);
			return count;
		}
		return it->second;
	}

	// add string with id, and return it's id.
	// if string exists, the old id is returned.
	// if id exists and its string is different, throws an exception.
	template <typename countable_t = int>
	inline countable_t add(_in const std::string& s, _in countable_t id) {
		int _id = static_cast<int>(id);
		if (read_only) THROW_GAENARI_ERROR("it cannot be added in read-only.");
		int count = static_cast<int>(ids.size());
		auto [it, inserted] = strings.insert({s, count});
		if (inserted) {
			if (_id == count) {
				ids.emplace_back(&it->first);
				return count+1;
			} 

			// resize to id, and set value.
			resize_ids(_id, count);
			ids[_id] = &it->first;
			return _id;
		}

		// string existed.
		if (it->second == _id) return _id;

		// duplicated string.
		if (_id >= count) {
			// new id assigned.
			resize_ids(_id, count);
			ids[_id] = &it->first;
			return it->second;
		}

		// test string equal.
		auto v = ids[_id];
		if (not v) THROW_GAENARI_INTERNAL_ERROR0;	// nullptr (empty value).
		if (*v != s) THROW_GAENARI_INTERNAL_ERROR0;
		return it->second;
	}

	// return -1 if not existed.
	inline int get_id(_in const std::string& s) const {
		auto it = strings.find(s);
		if (it == strings.end()) return -1;
		return it->second;
	}

	// return std::numeric_limits<size_t>::max() if not existed.
	inline size_t get_id_size_t(_in const std::string& s) const {
		auto it = strings.find(s);
		if (it == strings.end()) return std::numeric_limits<size_t>::max();
		return static_cast<size_t>(it->second);
	}

	// except if not existed.
	template <typename countable_t = int>
	inline const std::string& get_string(_in countable_t id) const {
		int _id = static_cast<int>(id);
		if (_id >= static_cast<int>(ids.size())) THROW_GAENARI_INTERNAL_ERROR0;
		auto& v = ids[_id];
		if (not v) {
			// nullptr (empty value).
			THROW_GAENARI_ERROR1("not found string table id(%0)", _id);
		}
		return *v;
	}

	// return empty string if not existed.
	template <typename countable_t = int>
	inline const std::string& get_string_noexept(_in countable_t id) const noexcept {
		try {
			return get_string(id);
		} catch(...) {
			return empty_string;
		}
	}

	// for convenience, `ref` is not const, but read_only is set to make it const.
	// so once a reference is made, the value cannot be changed.
	inline void reference_from(_in string_table& ref) {
		read_only = true;
		referencing = true;
		strings = ref._strings;
		ids = ref._ids;
	}

	inline void reference_from_const(_in const string_table* ref) {
		// seems unreasonable, but remove the const.
		// this is because there is no const in the main function.
		// no need to worry, const has been removed, but works as `read_only`.
		string_table* _ref = const_cast<string_table*>(ref);
		reference_from(*_ref);
	}

	// break the reference and copy.
	inline void copy_from_reference(void) {
		if (not referencing) return;
		// copy.
		_strings = strings;
		_ids = ids;
		referencing = false;
		read_only = false;
	}

	// copy assignment.
	inline string_table& operator=(const string_table& s) {
		if (this != &s) {
			this->read_only = s.read_only;
			this->referencing = s.referencing;
			this->_strings = s._strings;
			this->_ids = s._ids;
			if (this->referencing) {
				this->strings = s.strings;
				this->ids = s.ids;
			} else {
				this->strings = this->_strings;
				this->ids = this->_ids;
			}
		}
		return *this;
	}

	// max id.
	inline int get_last_id(void) const {
		int c = static_cast<int>(ids.size());
		if (c <= 0) return -1;
		if (not ids[static_cast<size_t>(c)-1]) THROW_GAENARI_INTERNAL_ERROR0;	// last item must not be nullptr.
		return c-1;
	}

	// compare operator for dataframe::append(...).
	bool operator==(const string_table& s) const {
		if ((strings == s.strings) and (ids == s.ids)) return true;
		return false;
	}

	bool operator!=(const string_table& s) const {
		return not (*this == s);
	}

protected:
	inline void resize_ids(_in int id, _in int count) {
		if (id < count) return;
		ids.resize(static_cast<size_t>(id) + 1);
	}

protected:
	bool read_only = false;
	bool referencing = false;
	const std::string empty_string;

	// object data content.
	// the good design is vector<optional<reference_wrapper<string>>>
	// but it requires additional memory.
	// if there are many strings, this is a cost, so use a pointer inevitably.
	//   (vector<optional<reference_wrapper<string>>> -> vector<string*>)
	std::map<std::string, int> _strings;	// string -> id
	std::vector<const std::string*> _ids;	// id -> string

	// reference pointer.
	std::map<std::string, int>& strings = _strings;	// string -> id
	std::vector<const std::string*>& ids = _ids;	// id -> string
};

} // comon
} // gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_STRING_TABLE_HPP
