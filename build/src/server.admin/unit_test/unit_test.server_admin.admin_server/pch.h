#pragma once

#define UNW_LOCAL_ONLY
#define _CGDK_NO_ASSERT_ON_BOUND_CHECK

#include "cgdk/sdk10/prefix.h"

#include "gtest/gtest.h"

#include <deque>
#include <optional>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "cgdk/sdk10/server.service.h"

using namespace CGDK;

// - admin server policies (tested)
#include "../../admin_server/admin_server.console/admin_server.policy.h"
