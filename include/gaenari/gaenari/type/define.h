#ifndef HEADER_GAENARI_GAENARI_TYPE_DEFINE_H
#define HEADER_GAENARI_GAENARI_TYPE_DEFINE_H

#define _in
#define _out
#define _in_out
#define _option
#define _option_in
#define _option_out

#ifdef _MSC_VER
#define PRETTY_FUNCTION __FUNCSIG__
#else
#define PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

// F("fmt %0 %1 %0", {1,"3"}) => "fmt 1 3 1"
#define F gaenari::common::f

#endif // HEADER_GAENARI_GAENARI_TYPE_DEFINE_H
