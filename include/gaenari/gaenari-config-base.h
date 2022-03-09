#ifndef HEADER_GAENARI_CONFIG_BASE_H
#define HEADER_GAENARI_CONFIG_BASE_H

// uses gaenari-config.h that are configured from cmake.
// this header file defines default values to avoid compilation errors when used without cmake.

// version is only meaningful through cmake.
#define GAENARI_VERSION_MAJOR 0
#define GAENARI_VERSION_MINOR 0
#define GAENARI_VERSION_PATCH 0
#define GAENARI_VERSION_TWEAK 0
#define GAENARI_VERSION			__QuOtE__(0.0.0)

#endif // HEADER_GAENARI_CONFIG_BASE_H
