// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

// Convenience header for the CLI and for experiments.
// Tests include the single header they are about, so a missing include fails there.

#include "aerotest/air_data_subsystem.hpp"
#include "aerotest/bus.hpp"
#include "aerotest/engine_subsystem.hpp"
#include "aerotest/fault_injector.hpp"
#include "aerotest/loopback_bus.hpp"
#include "aerotest/message.hpp"
#include "aerotest/results_writer.hpp"
#include "aerotest/scenario.hpp"
#include "aerotest/scenario_parser.hpp"
#include "aerotest/scenario_runner.hpp"
#include "aerotest/socketcan_bus.hpp"
#include "aerotest/status.hpp"
#include "aerotest/subsystem.hpp"
#include "aerotest/test_runner.hpp"
#include "aerotest/version.hpp"
