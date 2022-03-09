#ifndef HEADER_GAENARI_GAENARI_COMMON_CACHE_HPP
#define HEADER_GAENARI_GAENARI_COMMON_CACHE_HPP

namespace gaenari {
namespace common {

// supports memory-based (key, value) set/get.
// stores the count of get(key) calls.
// and when the number of keys increases to capacity, some cache entries are deleted.
// if the rank of count is less than `survive_size`, it will be deleted.
// and the conut value of the remaining items is set to zero.
//
// K : key type
// V : value type
// C : count type (default int)
template <typename K, typename V, typename C=int>
class cache {
public:
	cache() = delete;
	inline cache(_in size_t capacity, _in size_t survive_size) {
		if ((survive_size >= capacity) or (capacity < 4)) THROW_GAENARI_INTERNAL_ERROR0;
		this->capacity = static_cast<C>(capacity);
		this->survive_size = static_cast<C>(survive_size);
	}
	~cache() = default;

public:
	using callback_set = std::function<void(_in const K& k, _out V& v)>;

public:
	// get the copied value of target key.
	// if not found, calls cb to get the value.
	// calling 'set' checks capacity and deletes items if necessary.
	// ex)
	// auto v = cache.get("key", [](_in auto& k, _out auto& v) {
	//		// so long time.
	//		v = "value"
	// });
	inline const V get(_in const K& k, _in callback_set cb) {
		// mutex lock.
		std::lock_guard<std::recursive_mutex> l(mutex);

		// return copy.
		return _get(k, cb);
	}

	// get the value of target key.
	// if not found, calls cb to get the value.
	// calling 'set' checks capacity and deletes items if necessary.
	// ex)
	// auto& v = cache.get_ref("key", [](_in auto& k, _out auto& v) {
	//		// so long time.
	//		v = "value"
	// });
	//
	// when you get return as a reference, be careful.
	// if necessary, use get_mutex() to lock one more time.
	inline const V& get_ref(_in const K& k, _in callback_set cb) {
		// mutex lock.
		std::lock_guard<std::recursive_mutex> l(mutex);

		// return reference.
		return _get(k, cb);
	}

	// to pre-lock at the caller.
	// this is for more secure transactions.
	inline std::recursive_mutex& get_mutex(void) {
		return mutex;
	}

	// get mutable cache items.
	// call after lock.
	inline auto& get_items(void) {
		return items;
	}

	// erase.
	inline void erase(_in const K& k) {
		// mutex lock.
		std::lock_guard<std::recursive_mutex> l(mutex);
		items.erase(k);
	}

	// clear.
	inline void clear(void) {
		// mutex lock.
		std::lock_guard<std::recursive_mutex> l(mutex);
		items.clear();
	}

protected:
	inline const V& _get(_in const K& k, _in callback_set cb) {
		// find key.
		auto find = items.find(k);
		if (find != items.end()) {
			// found!
			auto& p = find->second;

			// add count.
			p.second++;

			// return value.
			return p.first;
		}

		// not found.
		// call callback to get value.
		V v;
		(cb)(k, v);

		C size = static_cast<C>(items.size());
		if (size < capacity) {
			// can afford.
			items[k] = std::make_pair(std::move(v), 1);
			return items[k].first;
		}

		// survive.
		std::vector<C> counts;
		for (const auto& item: items) {
			const auto& p = item.second;
			counts.push_back(p.second);
		}
		std::sort(counts.begin(), counts.end(), std::greater<C>());
		C survive = 0;
		C acc = 0;
		for (auto count: counts) {
			if (acc + count <= survive_size) {
				survive = count;
				acc += count;
				continue;
			}
			
			break;
		}

		// survive run.
		for (auto it=items.begin(); it != items.end(); ) {
			auto& p = it->second;
			auto& c = p.second;
			if (c >= survive) {
				// survived !
				c = 0;	// survived, but set count to zero.
				++it;
			} else {
				// died !
				items.erase(it++);
			}
		}

		// log warning.
		gaenari::logger::warn("cache refreshed.");

		// set and return value.
		items[k] = std::make_pair(std::move(v), 1);
		return items[k].first;
	}

protected:
	// cache data.
	// (key, (value, frequency)) map.
	std::unordered_map<K,std::pair<V,C>> items;
	std::recursive_mutex mutex;
	C capacity = 0;
	C survive_size = 0;
};

// ex)
// cache<std::string, std::string> c(5, 2);			-> 5 capacity, 2 survive.
// c.get("aaa", [](auto& k, auto& v) {v = "111";});	-> callback called.     ("aaa", ("111", 1))
// c.get("aaa", [](auto& k, auto& v) {v = "111";});	-> callback not called. ("aaa", ("111", 2))
// c.get("bbb", [](auto& k, auto& v) {v = "222";});	-> callback called.     ("aaa", ("111", 2)), ("bbb", ("222", 1))
// c.get("ccc", [](auto& k, auto& v) {v = "333";});	-> callback called.
// c.get("ddd", [](auto& k, auto& v) {v = "444";});	-> callback called.
// c.get("eee", [](auto& k, auto& v) {v = "555";});	-> callback called.
// c.get("fff", [](auto& k, auto& v) {v = "666";});	-> callback called. survive run. ("aaa", ("111", 0)), ("fff", ("666", 1))
// c.get("ggg", [](auto& k, auto& v) {v = "777";});

} // common
} // gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_CACHE_HPP
