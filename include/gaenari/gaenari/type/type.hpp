#ifndef HEADER_GAENARI_GAENARI_TYPE_BASE_HPP
#define HEADER_GAENARI_GAENARI_TYPE_BASE_HPP

namespace gaenari {
namespace type {

// type definition
//
// ----------+-------------------------------------------------------------------------------------------------
// type name | desc
// ----------+-------------------------------------------------------------------------------------------------
// variant   | a type that can use multiple data types based on std::variant.
//           | std::string is included, so the default sizeof(variant) is large(about 48 bytes).
//           | it can be used for type-independent function call like python.
//           | it is used to get the value of dataframe.
//           | std::string is returned as std::string on string data type column.
//           | it can be found out which type was used by calling the index() of the std::variant.
//           | * std::string - supported.
// ----------+-------------------------------------------------------------------------------------------------
// value_raw | a small type with a 8-byte size where primitive types(=POD, plain old data) are shared as union.
//           | defined c standard type, constructors, destructors and methods are not defined.
//           | it is the type of array allocated in dataframe.
//           | it is required to know which value of the shared variable to use.
//           | * std::string - not supported. (access the value_raw::index of an external std::string vector)
// ----------+-------------------------------------------------------------------------------------------------
// value_obj | value_raw is inconvenient to use.
//           | wrap some functions, and it can be used as a key of std::map.
//           | same as value_raw, size is 8.
//           | it is required to know which value of the shared variable to use.
//           | * std::string - not supported. (use the value_raw::index to access std::string vector)
// ----------+-------------------------------------------------------------------------------------------------
// value     | value_raw and value_obj are small size.
//           | but, it is necessary to know in advance what value to get.
//           | to solve this inconvenience, shared data and it's type are stored together.
//           | with new information added, the size becomes 16 bytes.
//           | * std::string - not supported. (use the value_raw::index to access std::string vector)
// ----------+-------------------------------------------------------------------------------------------------

// variant and type index
// using variant to function parameter, it seems like type-less function call without template.
using variant = std::variant<std::monostate,int,size_t,std::string,double,int64_t>;
enum  variant_type {variant_unknown = 0, variant_int = 1, variant_size_t = 2, variant_string = 3, variant_double = 4, variant_int64 = 5};

// dataframe primitive node type is standard c style union.
// its footprint is smaller(8byte) than variant(16byte~48byte).
// it is alloced by malloc, so do not add method. just c style.
struct value_raw {
	union {
		size_t  index;	// reference to external vector index.
		double  numeric_double;
		int     numeric_int32;
		int64_t numeric_int64;
	};
};

// value inherit from raw.
// it's class so it can add functions.
// no members added from value_raw.
struct value_obj : public value_raw {
	inline value_obj()  {this->index = 0;}
	inline value_obj(_in const value_raw& v)   {this->index = v.index;}
	inline value_obj(_in const size_t& v)      {this->index = v;}
	inline value_obj(_in const double& v)      {this->numeric_double = v;}
	inline value_obj(_in const int& v)         {this->numeric_int32 = v;}
	inline value_obj(_in const int64_t& v)     {this->numeric_int64 = v;}
	inline ~value_obj() = default;

	// assign
	inline value_obj& operator=(_in const value_obj& v) {this->index = v.index; return *this;}

	// cast
	inline operator int (void) const     {return this->numeric_int32;}
	inline operator double (void) const  {return this->numeric_double;}
	inline operator int64_t (void) const {return this->numeric_int64;}
	inline operator size_t (void) const  {return this->index;}

	// to use as a key of std::map, need to some operator overloading.
	inline bool const operator==(const value_obj& o) const {
		// shared memory, so compare only one item.
		return this->numeric_int64 == o.numeric_int64;
	}

	// to use value_obj as std::map key.
	// it's not sorting logic for all data types.
	// (just becase int64 is sorted, there is no 100% guarantee that the double on shared bytes will be sorted.)
	// if you wnat exact sorting by data type, use value.
	inline bool const operator<(const value_obj& o) const {
		// shared memory, so compare only one item.
		return this->numeric_int64 < o.numeric_int64;
	}
};

// value type
enum class value_type {value_type_unknown = 0, value_type_int = 1, value_type_int64 = 2, value_type_double = 3, value_type_size_t = 4};

// value inherit from raw.
// member 'type' added from value_raw.
// unlike value_raw and value_obj, value type is known.
// that is, it knows both value and it's type.
// but, 8 bytes are added because of the 'type' variable.
struct value : public value_raw {
	inline value() {this->index = 0; this->valueype = value_type::value_type_unknown;}
	inline value(_in const value_raw& v, _in value_type valueype): valueype{valueype} {this->index = v.index;}
	inline value(_in const value& v,     _in value_type valueype): valueype{valueype} {this->index = v.index;}
	inline value(_in const size_t& v):  valueype{value_type::value_type_size_t} {this->index = v;}
	inline value(_in const double& v):  valueype{value_type::value_type_double} {this->numeric_double = v;}
	inline value(_in const int& v):     valueype{value_type::value_type_int}    {this->numeric_int32 = v;}
	inline value(_in const int64_t& v): valueype{value_type::value_type_int64}  {this->numeric_int64 = v;}
	inline ~value() = default;

	// assign
	inline value& operator=(_in const value& v) {this->index = v.index; this->valueype = v.valueype; return *this;}

	// math
	inline value operator+ (const size_t& v) const {
		if (this->valueype != value_type::value_type_size_t) THROW_GAENARI_ERROR("type mis-match.");
		return value(v + this->index);
	}

	inline value operator+ (const double& v) const {
		if (this->valueype != value_type::value_type_double) THROW_GAENARI_ERROR("type mis-match.");
		return value(v + this->numeric_double);
	}

	inline value operator+ (const int& v) const {
		if (this->valueype != value_type::value_type_int) THROW_GAENARI_ERROR("type mis-match.");
		return value(v + this->numeric_int32);
	}

	inline value operator+ (const int64_t& v) const {
		if (this->valueype != value_type::value_type_int64) THROW_GAENARI_ERROR("type mis-match.");
		return value(v + this->numeric_int64);
	}

	inline value& operator+= (const size_t& v) {
		if (this->valueype != value_type::value_type_size_t) THROW_GAENARI_ERROR("type mis-match.");
		this->index += v;
		return *this;
	}

	inline value& operator+= (const double& v) {
		if (this->valueype != value_type::value_type_double) THROW_GAENARI_ERROR("type mis-match.");
		this->numeric_double += v;
		return *this;
	}

	inline value& operator+= (const int& v) {
		if (this->valueype != value_type::value_type_int) THROW_GAENARI_ERROR("type mis-match.");
		this->numeric_int32 += v;
		return *this;
	}

	inline value& operator+= (const int64_t& v) {
		if (this->valueype != value_type::value_type_int64) THROW_GAENARI_ERROR("type mis-match.");
		this->numeric_int64 += v;
		return *this;
	}

	// compare
	inline bool const operator==(const value& o) const {
		// shared memory, so compare only one item.
		return (this->numeric_int64 == o.numeric_int64) and (this->valueype == o.valueype);
	}

	inline bool const operator<(const value& o) const {
		if (this->valueype != o.valueype) THROW_GAENARI_ERROR("type mis-match.");
		if (this->valueype == value_type::value_type_size_t) return this->index < o.index;
		if (this->valueype == value_type::value_type_double) return this->numeric_double < o.numeric_double;
		if (this->valueype == value_type::value_type_int)    return this->numeric_int32 < o.numeric_int32;
		if (this->valueype == value_type::value_type_int64)  return this->numeric_int64 < o.numeric_int64;
		THROW_GAENARI_ERROR("invalid value type.");
		return false;
	}

	inline bool const operator<=(const value& o) const {
		if (this->valueype != o.valueype) THROW_GAENARI_ERROR("type mis-match.");
		if (this->valueype == value_type::value_type_size_t) return this->index <= o.index;
		if (this->valueype == value_type::value_type_double) return this->numeric_double <= o.numeric_double;
		if (this->valueype == value_type::value_type_int)    return this->numeric_int32 <= o.numeric_int32;
		if (this->valueype == value_type::value_type_int64)  return this->numeric_int64 <= o.numeric_int64;
		THROW_GAENARI_ERROR("invalid value type.");
		return false;
	}

	inline bool const operator>(const value& o) const {
		if (this->valueype != o.valueype) THROW_GAENARI_ERROR("type mis-match.");
		if (this->valueype == value_type::value_type_size_t) return this->index > o.index;
		if (this->valueype == value_type::value_type_double) return this->numeric_double > o.numeric_double;
		if (this->valueype == value_type::value_type_int)    return this->numeric_int32 > o.numeric_int32;
		if (this->valueype == value_type::value_type_int64)  return this->numeric_int64 > o.numeric_int64;
		THROW_GAENARI_ERROR("invalid value type.");
		return false;
	}

	inline bool const operator>=(const value& o) const {
		if (this->valueype != o.valueype) THROW_GAENARI_ERROR("type mis-match.");
		if (this->valueype == value_type::value_type_size_t) return this->index >= o.index;
		if (this->valueype == value_type::value_type_double) return this->numeric_double >= o.numeric_double;
		if (this->valueype == value_type::value_type_int)    return this->numeric_int32 >= o.numeric_int32;
		if (this->valueype == value_type::value_type_int64)  return this->numeric_int64 >= o.numeric_int64;
		THROW_GAENARI_ERROR("invalid value type.");
		return false;
	}

	// cast
	inline operator int (void) const     {if (this->valueype != value_type::value_type_int)    THROW_GAENARI_ERROR("type mis-match."); return this->numeric_int32;}
	inline operator double (void) const  {if (this->valueype != value_type::value_type_double) THROW_GAENARI_ERROR("type mis-match."); return this->numeric_double;}
	inline operator int64_t (void) const {if (this->valueype != value_type::value_type_int64)  THROW_GAENARI_ERROR("type mis-match."); return this->numeric_int64;}
	inline operator size_t (void) const  {if (this->valueype != value_type::value_type_size_t) THROW_GAENARI_ERROR("type mis-match."); return this->index;}

	// additional member variable
	value_type valueype;
};

// static_assert(false) is mal-formed. use false instead.
// ex)
// if constexpr (std::is_same<T,...>::value) { ... } else {
//     static_assert(false, "");
//     ->
//     static_assert(dependent_false_v<T>, "");
template<typename> inline constexpr bool dependent_false_v = false;

} // namespace type
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_TYPE_BASE_HPP
