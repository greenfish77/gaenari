#ifndef HEADER_GAENARI_GAENARI_COMMON_SPAN_HPP
#define HEADER_GAENARI_GAENARI_COMMON_SPAN_HPP

namespace gaenari {
namespace common {

// typical span for access the contiguous memory like vector.
//
// int t[] = {1,2,3,4,5}
// span<int> s(t, 5);
// auto  a = s[0];					// return 1
// auto& b = s[0];					// return 1, memory shared, mutable
// b = 999;							// 1 -> 999, value changed
// std::sort(s.begin(), s.end());	// sortable
//
// int t[] = {1,2,3,4,5}
// span<const int> s(t, 5);
// auto  a = s[0];					// return 1
// auto& b = s[0];					// return 1, memory shared, but immutable
// b = 999;							// compile error
//
// iterator
// for (auto& i: s) {
//     ... (mutable, memory shared i)
// for (const auto& i: s) {
//     ... (immutable, memory shared i)
// for (auto i=s.begin(); i!=s.end(); i++) {
//     ... (mutable, memory shared *i)
// for (auto i=s.cbegin(); i!=s.cend(); i++) {
//     ... (immutable, memory shared *i)
//
// offset, step
// int t[] = {19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
// span<int> s(t, 5, 3, 2);	// length : 5, offset = 3, step = +2
// for (auto& v: s) std::cout << v << ' ';
// ==> 16 14 12 10 8 
//
// int t[] = {19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
// span<int> s(t, 5, 3, 2);	// length : 5, offset = 3, step = +2
// std::sort(s.begin(), s.end());
// for (auto& v: s) std::cout << v << ' ';
// ==> 8 10 12 14 16
// t : {19, 18, 17, 8, 15, 10, 13, 12, 11, 14, 9, 16, 7, 6, 5, 4, 3, 2, 1, 0}

template<typename T>
class span {
public:
	inline span(T* ptr, size_t len): _ptr{ptr}, _len{len} {}
	inline span(T* ptr, size_t len, size_t first_offset, std::ptrdiff_t step): _ptr{&ptr[first_offset]}, _len{len}, _step{step} {}
	inline ~span() = default;
public:
	inline T& operator[](int i) {
		return this->_ptr[i];
	}

	inline const T& operator[](int i) const {
		return this->_ptr[i];
	}

	inline size_t size(void) {
		return this->_len;
	}

	inline T* data(void) {
		return this->_ptr;
	}

	inline const T* cdata(void) {
		return this->_ptr;
	}

public:
	// iterator
	// support std::sort.
	struct iterator {
		using iterator_category = std::random_access_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using valueype        = T;
		using pointer           = T*;
		using reference         = T&;

		inline iterator() : _ptr(nullptr) {}
		inline iterator(pointer ptr, std::ptrdiff_t step): _ptr{ptr}, _step{step}  {if (step <= 0) THROW_GAENARI_ERROR("invalid step.");}
		inline iterator(const iterator &rhs): _ptr(rhs._ptr), _step{rhs._step} {}

		// misc
		inline iterator& operator=(T* rhs)						{ this->_ptr  = rhs; return *this; }
		inline iterator& operator=(const iterator& rhs)			{ this->_ptr  = rhs._ptr; this->_step = rhs._step; return *this; }
		inline iterator& operator+=(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) { this->_ptr += rhs; } else { this->_ptr += rhs * this->_step; } return *this; }
		inline iterator& operator-=(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) { this->_ptr -= rhs; } else { this->_ptr -= rhs * this->_step; } return *this; }
		inline reference operator*() const						{ return *this->_ptr; }
		inline pointer  operator->()							{ return  this->_ptr; }
		inline reference operator[](const int& rhs)				{ if (this->_step == 1) return this->_ptr[rhs]; return this->_ptr[rhs * this->_step]; }

		// arithmetic
		inline iterator& operator++()							{ if (this->_step == 1) { this->_ptr++; } else { this->_ptr += this->_step; } return *this; }
		inline iterator& operator--()							{ if (this->_step == 1) { this->_ptr--; } else { this->_ptr -= this->_step; } return *this; }
		inline iterator operator++(int)							{ iterator tmp = *this; ++(*this); return tmp; }
		inline iterator operator--(int)							{ iterator tmp = *this; --(*this); return tmp; }
		inline iterator operator+(const iterator& rhs)			{ return iterator(this->_ptr + rhs._ptr); }
		inline iterator operator+(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) return iterator(this->_ptr + rhs, 1); return iterator(this->_ptr + rhs * this->_step, this->_step); }
		friend inline iterator operator+(const std::ptrdiff_t& lhs, const iterator& rhs)	{ return iterator(lhs + rhs._ptr); }
		friend inline iterator operator-(const iterator& rhs, const std::ptrdiff_t lhs)		{ if (rhs._step == 1) return iterator(rhs._ptr - lhs, 1); return iterator(rhs._ptr - lhs * rhs._step, rhs._step); }
		friend inline std::ptrdiff_t operator-(const iterator& rhs, const iterator& lhs)	{ return (rhs._ptr - lhs._ptr) / rhs._step; }

		// comparision
		inline bool operator==(const iterator& rhs)	{ return this->_ptr == rhs._ptr; }
		inline bool operator!=(const iterator& rhs)	{ return this->_ptr != rhs._ptr; }
		inline bool operator>(const iterator& rhs)	{ return this->_ptr >  rhs._ptr; }
		inline bool operator<(const iterator& rhs)	{ return this->_ptr <  rhs._ptr; }
		inline bool operator>=(const iterator& rhs)	{ return this->_ptr >= rhs._ptr; }
		inline bool operator<=(const iterator& rhs)	{ return this->_ptr <= rhs._ptr; }

	protected:
		pointer _ptr = nullptr;
		std::ptrdiff_t _step = 1;
	};

	inline iterator begin() { return iterator(this->_ptr, this->_step); }
	inline iterator end()   { return iterator(this->_ptr + this->_len * this->_step, this->_step); }

public:
	// const iterator
	struct const_iterator {
		using iterator_category = std::random_access_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using valueype        = const T;
		using pointer           = const T*;
		using reference         = const T&;

		inline const_iterator() : _ptr(nullptr) {}
		inline const_iterator(pointer ptr, std::ptrdiff_t step): _ptr{ptr}, _step{step}  {if (step <= 0) THROW_GAENARI_ERROR("invalid zero step.");}
		inline const_iterator(const const_iterator &rhs): _ptr(rhs._ptr), _step{rhs._step} {}

		// misc
		inline const_iterator& operator=(T* rhs)						{ this->_ptr  = rhs; return *this; }
		inline const_iterator& operator=(const const_iterator& rhs)		{ this->_ptr  = rhs._ptr; this->_step = rhs._step; return *this; }
		inline const_iterator& operator+=(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) { this->_ptr += rhs; } else { this->_ptr += rhs * this->_step; } return *this; }
		inline const_iterator& operator-=(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) { this->_ptr -= rhs; } else { this->_ptr -= rhs * this->_step; } return *this; }
		inline reference operator*() const								{ return *this->_ptr; }
		inline pointer  operator->()									{ return  this->_ptr; }
		inline reference operator[](const int& rhs)						{ return this->_ptr[rhs * this->_step]; }

		// arithmetic
		inline const_iterator& operator++()							{ if (this->_step == 1) { this->_ptr++; } else { this->_ptr+=this->_step; } return *this; }
		inline const_iterator& operator--()							{ if (this->_step == 1) { this->_ptr--; } else { this->_ptr-=this->_step; } return *this; }
		inline const_iterator operator++(int)						{ const_iterator tmp = *this; ++(*this); return tmp; }
		inline const_iterator operator--(int)						{ const_iterator tmp = *this; --(*this); return tmp; }
		inline const_iterator operator+(const const_iterator& rhs)	{ return const_iterator(this->_ptr + rhs._ptr); }
		inline const_iterator operator+(const std::ptrdiff_t& rhs)	{ if (this->_step == 1) return iterator(this->_ptr + rhs, 1); return const_iterator(this->_ptr + rhs * this->_step, this->_step); }
		friend inline const_iterator operator+(const std::ptrdiff_t& lhs, const const_iterator& rhs)	{ return const_iterator(lhs + rhs._ptr); }
		friend inline const_iterator operator-(const const_iterator& rhs, const std::ptrdiff_t lhs)		{ if (rhs._step == 1) return const_iterator(rhs._ptr - lhs, 1); return const_iterator(rhs._ptr - lhs * rhs._step, rhs._step); }
		friend inline std::ptrdiff_t operator-(const const_iterator& rhs, const const_iterator& lhs)	{ return (rhs._ptr - lhs._ptr) / rhs._step; }

		// comparision
		inline bool operator==(const const_iterator& rhs)	{ return this->_ptr == rhs._ptr; }
		inline bool operator!=(const const_iterator& rhs)	{ return this->_ptr != rhs._ptr; }
		inline bool operator>(const const_iterator& rhs)	{ return this->_ptr >  rhs._ptr; }
		inline bool operator<(const const_iterator& rhs)	{ return this->_ptr <  rhs._ptr; }
		inline bool operator>=(const const_iterator& rhs)	{ return this->_ptr >= rhs._ptr; }
		inline bool operator<=(const const_iterator& rhs)	{ return this->_ptr <= rhs._ptr; }

	protected:
		pointer _ptr = nullptr;
		std::ptrdiff_t _step = 1;
	};

	inline const_iterator cbegin() { return const_iterator(this->_ptr, this->_step); }
	inline const_iterator cend()   { return const_iterator(this->_ptr + this->_len * this->_step, this->_step); }

protected:
	// span has only beginning pointer, length, and step.
	T* _ptr = nullptr;
	size_t _len = 0;
	std::ptrdiff_t _step = 1;
};

// non-typical span for access the random selected memory like vector.
//
// int t[] = {1,3,5,2,4};
// selected_span<int> s(t, {3,1,1,4});		// index 3=>2, 1=>3, 1=>3, 4=>4, access {2,3,3,4}
// auto  a = s[1];								// return 3
// auto& b = s[1];								// return 3, memory shared, mutable.
// b = 999;										// 3 -> 999, value changed (s[2] is also assign to 999)
// std::sort(s.begin(), s.end());				// not support sort. compile error.
//
// int t[] = {1,3,5,2,4};
// selected_span<const int> s(t, {3,1,1,4});	// immutable const span
// auto  a = s[1];								// return 3
// auto& b = s[1];								// return 3, memory shared, but immutable.
// b = 999;										// compile error
//
// int t[] = {1,3,5,2,4};
// selected_span<int> s(t, 5);				// same as span. does not have internal index vector. logically {0,1,2,3,4}
//
// iterator
// for (auto& i: s) {
//     ... (mutable, memory shared i)
// for (const auto& i: s) {
//     ... (immutable, memory shared i)
// for (auto i=s.begin(); i!=s.end(); i++) {
//     ... (mutable, memory shared *i)
// for (auto i=s.cbegin(); i!=s.cend(); i++) {
//     ... (immutable, memory shared *i)
template<typename T>
class selected_span {
public:
	inline selected_span() {}
	inline selected_span(const std::vector<size_t>& indexes): _ptr{nullptr}, _ptr_move_source{nullptr}, _indexes_body{indexes}, _indexes{&_indexes_body} {}
	inline selected_span(T* ptr, const std::vector<size_t>& indexes): _ptr{ptr}, _ptr_move_source{ptr}, _indexes_body{indexes}, _indexes{&_indexes_body} {}
	inline selected_span(T* ptr, size_t len): _ptr{ptr}, _ptr_move_source{ptr}, _len{len}, _all{true}, _indexes_body{}, _indexes{&_indexes_body} {}
	inline ~selected_span() = default;

public:
	inline T& operator[](int i) {
		if (this->_all) return this->_ptr[i];
		return this->_ptr[(*this->_indexes)[i]];
	}

	inline const T& operator[](int i) const {
		if (this->_all) return this->_ptr[i];
		return this->_ptr[(*this->_indexes)[i]];
	}

	inline void set_pointer(T* ptr) {
		this->_ptr = ptr;
		this->_ptr_move_source = ptr;
	}

	inline const T* get_pointer(void) {
		return this->_ptr;
	}

	inline void move_pointer(std::ptrdiff_t item_distance) {
		this->_ptr = this->_ptr_move_source + item_distance;
	}

	inline std::ptrdiff_t get_move_distance(void) {
		return this->_ptr - this->_ptr_move_source;
	}

	inline void indexes(const std::vector<size_t>& indexes) {
		this->_indexes = indexes;
	}

	inline const std::vector<size_t>& indexes(void) const {
		return *(this->_indexes);
	}

	inline void set_indexes_reference(const std::vector<size_t>& indexes) {
		if (this->_all) {
			this->_all = false;
			this->_len = 0;
		}
		// does not copy index. just point to caller's indexes.
		this->_indexes_body.clear();
		this->_indexes = &indexes;
	}

	inline void set_indexes(const std::vector<size_t>& indexes) {
		if (this->_all) {
			this->_all = false;
			this->_len = 0;
		}
		// copy index
		this->_indexes_body = indexes;
		this->_indexes = &this->_indexes_body;
	}

	// do not use indexes.
	inline void set_len(size_t len) {
		this->_indexes = nullptr;
		this->_indexes_body.clear();
		this->_len = len;
		this->_all = true;
	}

	inline size_t size(void) const {
		if (this->_all) return this->_len;
		return this->_indexes->size();
	}

public:
	// simple iterator
	struct iterator {
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using valueype        = T;
		using pointer           = T*;
		using reference         = T&;

		inline iterator(selected_span* span): _span{span}  {if (!span) this->_pos = -1;}

		inline bool operator!=(const iterator& rhs)	{ return this->_pos != rhs._pos; }
		inline reference operator*() const {return this->value();}

		inline iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
		inline iterator& operator++() {
			if (this->_pos == -1) return *this; // eof + 1 = eof
			this->_pos++;
			// eof check
			if (this->_span->_all) {
				if (this->_pos >= (int)(this->_span->_len)) this->_pos = -1;
			}
			else if (this->_pos >= (int)(this->_span->_indexes->size())) this->_pos = -1;
			return *this;
		}

		inline iterator operator+(const int& rhs) {
			iterator tmp = *this;
			if (tmp._pos == -1) tmp._pos = (int)tmp._span->_indexes->size();
			tmp._pos += rhs;
			if (tmp._pos < 0) tmp._pos = -1;
			else {
				// eof check
				if (tmp._span->_all) {
					if (tmp._pos >= (int)(tmp._span->_len)) tmp._pos = -1;
				}
				if (tmp._pos >= (int)(tmp._span->_indexes->size())) tmp._pos = -1;
			}
			return tmp;
		}

		inline iterator operator-(const int& rhs) {
			return operator+(-rhs);
		}

		inline reference value(void) const {
			return (*this->_span)[this->_pos];
		}

	protected:
		selected_span* _span = nullptr;
		int _pos = 0;
	};

	inline iterator begin() {return iterator(this);}
	inline iterator end()   {return iterator(nullptr);}

public:
	// simple const iterator
	struct const_iterator {
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using valueype        = const T;
		using pointer           = const T*;
		using reference         = const T&;

		inline const_iterator(selected_span* span): _span{span}  {if (not span) this->_pos = -1;}

		inline bool operator!=(const const_iterator& rhs)	{ return this->_pos != rhs._pos; }
		inline reference operator*() const {return this->value();}

		inline const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }
		inline const_iterator& operator++() {
			if (this->_pos == -1) return *this; // eof + 1 = eof
			this->_pos++;
			if (this->_pos >= (int)(this->_span->_indexes->size())) this->_pos = -1;	// eof check
			return *this;
		}

		inline const_iterator operator+(const int& rhs) {
			const_iterator tmp = *this;
			if (tmp._pos == -1) tmp._pos = (int)tmp._span->_indexes->size();
			tmp._pos += rhs;
			if (tmp._pos < 0) tmp._pos = -1;
			else if (tmp._pos >= (int)(tmp._span->_indexes->size())) tmp._pos = -1;	// eof check
			return tmp;
		}

		inline const_iterator operator-(const int& rhs) {
			return operator+(-rhs);
		}

		inline reference value(void) const {
			return (*this->_span)[this->_pos];
		}

	protected:
		selected_span* _span = nullptr;
		int _pos = 0;
	};

	inline const_iterator cbegin() {return const_iterator(this);}
	inline const_iterator cend()   {return const_iterator(nullptr);}

protected:
	// selected_span has beginning pointer and indexes to choose.
	T* _ptr = nullptr;
	T* _ptr_move_source = nullptr;
	const std::vector<size_t>* _indexes = nullptr;
	std::vector<size_t>  _indexes_body;
	int t = 0;
	bool _all = false;
	size_t _len = 0;
};

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_SPAN_HPP
