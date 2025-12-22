#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef EXPORT
#ifdef _MSC_VER
#define API __declspec(dllexport)
#else
#define API __attribute__(visibility("default"))
#endif
#else
#ifdef _MSC_VER
#define API __declspec(dllimport)
#else
#define API
#endif
#endif