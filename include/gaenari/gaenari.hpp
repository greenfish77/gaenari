#ifndef HEADER_GAENARI_HPP
#define HEADER_GAENARI_HPP

// configuration.
// __has_include: c++17 standard.
#if __has_include("gaenari-config.h")
#include "gaenari-config.h"
#else
// no config heaer file.
// recommend to build with `cmake ..`
#pragma message ("[WARNING] compile without cmake.")
#include "gaenari-config-base.h"
#endif

// include all gaenari header files.
#include "gaenari/gaenari.hpp"

// include all supul header files.
#include "supul/supul.hpp"

#endif // HEADER_OUTER_GAENARI_HPP
