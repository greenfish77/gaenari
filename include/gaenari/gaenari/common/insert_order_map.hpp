#ifndef HEADER_GAENARI_GAENARI_COMMON_INSERT_ORDER_MAP_HPP
#define HEADER_GAENARI_GAENARI_COMMON_INSERT_ORDER_MAP_HPP

namespace gaenari {
namespace common {

// map with insertion order iteration.
template<typename key_t, typename value_t, typename countable_t = unsigned int>
struct insert_order_map {
	using key_type    = key_t;
	using mapped_type = value_t;
protected:
	// forward declaration.
	struct comp;

	// internal variables.
	//   - m(map with value)
	//   - o(map with order)
	//   - c(comparator of m)
	//   - i(insertion count)
	std::map<key_t, value_t, comp>* m = nullptr;
	std::map<std::reference_wrapper<const key_t>, countable_t, std::less<const key_t>> o;
	comp c;
	countable_t i = 0;

public:
	inline insert_order_map() {
		// build a new m with comparator c.
		// comparator c can link to this.
		c.parent = this;
		m = new std::map<key_t, value_t, comp>(c);
	}

	// insert_order_map<K, V> m{{k1,v1}, {k2,v2}, ...}
	inline insert_order_map(const std::initializer_list<std::pair<key_t, value_t>>& v) {
		c.parent = this;
		m = new std::map<key_t, value_t, comp>(c);

		// hard-copy.
		for (const auto& it: v) {
			(*this)[it.first] = it.second;
		}
	}

	inline insert_order_map(const insert_order_map& s) {
		// do assign.
		c.parent = this;
		m = new std::map<key_t, value_t, comp>(c);
		*this = s;
	}

	inline ~insert_order_map() {
		if (m) delete m;
		m = nullptr;
	}

	// used when accessing m from outside.
	// you can call some methods of std::map(read only).
	// reference) auto& d = m.get_map();
	// copy)      auto  d = m.get_map();
	inline const auto& get_map(void) const {
		return *m;
	}

	// assign.
	// map = map.
	inline insert_order_map& operator=(const insert_order_map& s) {
		// clear.
		this->clear();

		// hard-copy.
		// if `*this->m = *s->m` is used, the comparator in map is also copied,
		// and then inserts are incorrectly passed.
		// so do hard-copy.
		for (const auto& it: (*s.m)) {
			(*this)[it.first] = it.second;
		}
		
		// count copy.
		this->i = s.i;
		
		return *this;
	}

	// assign.
	// map = {{...}}
	inline insert_order_map& operator=(const std::initializer_list<std::pair<key_t, value_t>>& v) {
		// clear.
		this->clear();

		// hard-copy.
		for (const auto& it: v) {
			(*this)[it.first] = it.second;
		}

		return *this;
	}

	// move.
	inline insert_order_map& operator=(insert_order_map&& s) noexcept {
		// a perfect `move` is not supported(do hard-copy).
		// if `*this->m = std::move(*s.m);` is used, map pointer in comparator(=comp::parent) is also copied,
		// and then insertions are incorrectly passed.
		// so do hard-copy.
		//
		// is there a way to dynamically change the map's comparator contents?
		// if someone finds a way, we can implement a more perfect move.

		// just call `assign`(hard-copy).
		(*this) = s;

		// count copy.
		this->i = s.i;

		// after assign, clear s.
		s.clear();

		return *this;
	}

	// iterators.
	// warning)
	//    since the map must always be managed with o,
	//    do not use it to change the structure.
	using iterator       = typename std::map<key_t, value_t, comp>::iterator;
	using const_iterator = typename std::map<key_t, value_t, comp>::const_iterator;
	inline auto begin(void)        { return this->m->begin();   }
	inline auto end(void)          { return this->m->end();     }
	inline auto begin(void) const  { return this->m->cbegin();  }
	inline auto end(void)   const  { return this->m->cend();    }
	inline auto cbegin(void)       { return this->m->cbegin();  }
	inline auto cend(void)         { return this->m->cend();    }
	inline auto rbegin(void)       { return this->m->rbegin();  }
	inline auto rend(void)         { return this->m->rend();    }
	inline auto rbegin(void) const { return this->m->rbegin();  }
	inline auto rend(void)   const { return this->m->rend();    }
	inline auto crbegin(void)      { return this->m->crbegin(); }
	inline auto crend(void)        { return this->m->crend();   }

	// map api.

	// clear.
	inline void clear(void) {
		m->clear();
		o.clear();
		i = 0;
	}

	// empty.
	inline bool empty(void) const {
		return m->empty();
	}

	// access insert_order_map[k].
	inline value_t& operator[](key_t const& k) {
		const auto [it, inserted] = (*m).insert({k, value_t()});
		if (not inserted) {
			// k exsited. `it` is found iteration.
			return it->second;
		}

		// not found. `it` is inserted iteration.
		// set key's order.
		// if o.count() is used instead of i++, it my be duplicated in case of erase.
		// use unique increment values as count.
		o[it->first] = static_cast<countable_t>(i++);
		return it->second;
	}

	// insert(define only one typical use).
	// logic is the same as assign.
	inline std::pair<iterator, bool> insert(std::pair<key_t, value_t> p) {
		auto ret = (*m).insert(p);
		if (not ret.second) return ret;
		o[ret.first->first] = static_cast<countable_t>(i++);
		return ret;
	}

	inline iterator find(const key_t& k) {
		return m->find(k);
	}

	inline const_iterator find(const key_t& k) const {
		return m->find(k);
	}

	// erase one key.
	inline size_t erase(const key_t& k) {
		auto find = o.find(k);
		size_t ret = m->erase(k);
		if (find != o.end()) {
			o.erase(find);
		}
		return ret;
	}

	// size.
	inline size_t size(void) const {
		return m->size();
	}

	// count.
	inline size_t count(void) const {
		return m->count();
	}

	// bonus 1.
	// return order by key.
	// if key is not found, return {}(std::nullopt).
	inline std::optional<size_t> order(const key_t& k) const {
		auto find = o.find(k);
		if (find == o.end()) return {};
		return find->second;
	}

	// bonus 2.
	// return key by order.
	// if order is out-of-range, return {}(std::nullopt).
	inline std::optional<std::reference_wrapper<const key_t>> key(size_t order) const {
		// `o` is not vector, so it is O(n), not O(1).
		for (const auto& i: o) {
			auto& k = i.first;
			auto& v = i.second;
			if (v == order) return k;
		}
		return {};
	}

	// comparator.
protected:
	struct comp {
		// comparator can link to parent.
		insert_order_map* parent = nullptr;

		// determines the order of keys.
		// usually `return a < b` is used.
		// here, it is determined by the insert order of the key, not the value of the key.
		inline bool operator() (const key_t& a, const key_t& b) const {
			// default to maximum index value;
			size_t index_a = std::numeric_limits<size_t>::max();
			size_t index_b = std::numeric_limits<size_t>::max();

			// find the order of each a and b. 
			// if the order has not been inserted yet, the default(maximum) is remained.
			auto find_a = (parent)->o.find(a);
			auto find_b = (parent)->o.find(b);
			if (find_a != (parent)->o.end()) index_a = find_a->second;
			if (find_b != (parent)->o.end()) index_b = find_b->second;

			// returned by the input order, not the value of the key.
			return index_a < index_b;
		}
	};
};

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_INSERT_ORDER_MAP_HPP
