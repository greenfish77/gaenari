#ifndef HEADER_GAENARI_GAENARI_HPP
#define HEADER_GAENARI_GAENARI_HPP

// c library.
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#ifdef _WIN32
// windows version.
#include <io.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else // _WIN32
// linux version.
#include <unistd.h>
#include <sys/file.h>
#endif // _WIN32

// std c++17 library.
#include <stdexcept>
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sstream>
#include <string_view>
#include <fstream>
#include <functional>
#include <utility>
#include <optional>
#include <unordered_map>
#include <tuple>
#include <cctype>
#include <iterator>
#include <iostream>
#include <random>
#include <numeric>
#include <set>
#include <algorithm>
#include <limits>
#include <ostream>
#include <regex>
#include <stack>
#include <mutex>

// most basic header.
#include "gaenari/gaenari/type/define.h"
#include "gaenari/gaenari/exceptions/exceptions.hpp"
#include "gaenari/gaenari/type/type.hpp"
#include "gaenari/gaenari/logger/logger.hpp"

// header include for successful compilation.
// although classified as a category,
// there are some .hpps that are mis-classified by dependency order.

// high priority.
#include "gaenari/gaenari/dataset/dataframe.type.hpp"
#include "gaenari/gaenari/method/decision_tree/decision_tree.type.hpp"
#include "gaenari/gaenari/method/stringfy/stringfy.type.hpp"
#include "gaenari/gaenari/common/misc.hpp"

// common classs.
#include "gaenari/gaenari/common/insert_order_map.hpp"
#include "gaenari/gaenari/common/json.hpp"
#include "gaenari/gaenari/common/named_mutex.hpp"
#include "gaenari/gaenari/common/span.hpp"
#include "gaenari/gaenari/common/string_table.hpp"
#include "gaenari/gaenari/common/variant.hpp"
#include "gaenari/gaenari/common/property.hpp"
#include "gaenari/gaenari/common/cache.hpp"

// common library.
#include "gaenari/gaenari/common/time.hpp"

// utility.
#include "gaenari/gaenari/dataset/csv.hpp"
#include "gaenari/gaenari/dataset/repository.hpp"
#include "gaenari/gaenari/dataset/dataframe.hpp"
#include "gaenari/gaenari/dataset/dataset.hpp"
#include "gaenari/gaenari/method/decision_tree/decision_tree.util.hpp"
#include "gaenari/gaenari/method/decision_tree/decision_tree.engine.hpp"
#include "gaenari/gaenari/method/stringfy/stringfy.style.hpp"
#include "gaenari/gaenari/method/stringfy/stringfy.util.hpp"
#include "gaenari/gaenari/method/stringfy/stringfy.dataframe.hpp"
#include "gaenari/gaenari/method/stringfy/stringfy.tree.hpp"

// functional classes.
#include "gaenari/gaenari/method/decision_tree/decision_tree.hpp"

#endif // HEADER_GAENARI_GAENARI_HPP
