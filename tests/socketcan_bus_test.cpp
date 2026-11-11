// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/fault_injector.hpp"
#include "aerotest/socketcan_bus.hpp"

using aerotest::Error;
using aerotest::FaultInjector;
using aerotest::SocketCanBus;
using aerotest::Status;

// These tests do not need a CAN interface. The round trip that does is in
// vcan_bus_test.cpp and is labeled needs-vcan.

TEST(SocketCanBus, StoresTheInterfaceName) {
    const SocketCanBus bus{"vcan0"};
    EXPECT_EQ(bus.name(), "vcan0");
}

TEST(SocketCanBus, StartsClosed) {
    const SocketCanBus bus{"can0"};
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.fault_injector(), nullptr);
}

TEST(SocketCanBus, FaultSlotRoundTrips) {
    SocketCanBus bus{"vcan0"};
    FaultInjector injector;
    bus.set_fault_injector(&injector);
    EXPECT_EQ(bus.fault_injector(), &injector);
    bus.set_fault_injector(nullptr);
    EXPECT_EQ(bus.fault_injector(), nullptr);
}

TEST(SocketCanBus, DISABLED_SendBeforeOpen) {
    SocketCanBus bus{"vcan0"};
    aerotest::Message message;
    message.id = 1;
    const Status status = bus.send(message);
    EXPECT_EQ(status.code, Error::NotOpen);
    EXPECT_FALSE(bus.is_open());
}

TEST(SocketCanBus, DISABLED_CloseWhenClosedIsOk) {
    SocketCanBus bus{"vcan0"};
    const Status status = bus.close();
    EXPECT_TRUE(status.ok()) << status.detail;
    EXPECT_FALSE(bus.is_open());
}

TEST(SocketCanBus, DISABLED_RejectsEmptyName) {
    SocketCanBus bus{""};
    const Status status = bus.open();
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_FALSE(bus.is_open());
}
