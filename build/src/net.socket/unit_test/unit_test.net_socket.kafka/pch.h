#pragma once

// configure)
#define UNW_LOCAL_ONLY
#define _CGDK_NO_ASSERT_ON_BOUND_CHECK

#include "cgdk/sdk10/prefix.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

// - kafka (librdkafka: vcpkg on windows, 'librdkafka-dev' on linux)
#include "cgdk/sdk10/net.socket.kafka.h"

// - the mock cluster of librdkafka (no broker is needed for the tests. rdkafka_mock.h needs the C api first)
#include <librdkafka/rdkafka.h>
#include <librdkafka/rdkafka_mock.h>

using namespace CGDK;
