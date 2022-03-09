#ifndef HEADER_GAENARI_SUPUL_HPP
#define HEADER_GAENARI_SUPUL_HPP

// supul uses gaenari.
#include "../gaenari/gaenari.hpp"

// supul uses c++17 filesystem.
#include <filesystem>

// include std library.
#include <array>

// supul .hpps in dependency order.
#include "type/type.hpp"

// exceptions.
#include "exceptions/exceptions.hpp"

// high priority.
#include "common/misc.hpp"
#include "common/util.hpp"
#include "common/function_logger.hpp"
#include "common/gnuplot.hpp"

// db.
#include "db/schema.hpp"
#include "db/base.hpp"
#include "db/transaction_guard.hpp"
#include "db/sqlite/sqlite.type.hpp"
#include "db/sqlite/sqlite.hpp"

// supul.
#include "supul/supul.hpp"

#endif // HEADER_GAENARI_SUPUL_HPP
