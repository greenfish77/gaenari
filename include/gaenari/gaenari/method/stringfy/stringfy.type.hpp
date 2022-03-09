#ifndef HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TYPE_HPP
#define HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TYPE_HPP

namespace gaenari {
namespace method {
namespace stringfy {

enum class type {
	unknown			= 0,
	text_plain		= 1,	// text/plain
	text_colortag	= 2,	// text/colortag (text/plain with logger color tags.)
};

} // stringfy
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TYPE_HPP
