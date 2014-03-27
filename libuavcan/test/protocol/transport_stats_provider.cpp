/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#include <gtest/gtest.h>
#include <uavcan/protocol/transport_stats_provider.hpp>
#include "helpers.hpp"


TEST(TransportStatsProvider, Basic)
{
    InterlinkedTestNodesWithSysClock nodes;

    uavcan::TransportStatsProvider tsp(nodes.a);

    uavcan::GlobalDataTypeRegistry::instance().reset();
    uavcan::DefaultDataTypeRegistrator<uavcan::protocol::GetTransportStats> _reg1;

    ASSERT_LE(0, tsp.start());

    ServiceClientWithCollector<uavcan::protocol::GetTransportStats> tsp_cln(nodes.b);

    /*
     * First request
     */
    ASSERT_LE(0, tsp_cln.call(1, uavcan::protocol::GetTransportStats::Request()));
    ASSERT_LE(0, nodes.spinBoth(uavcan::MonotonicDuration::fromMSec(10)));

    ASSERT_TRUE(tsp_cln.collector.result.get());
    ASSERT_TRUE(tsp_cln.collector.result->isSuccessful());
    ASSERT_EQ(0, tsp_cln.collector.result->response.transfer_errors);
    ASSERT_EQ(1, tsp_cln.collector.result->response.transfers_rx);
    ASSERT_EQ(0, tsp_cln.collector.result->response.transfers_tx);
    ASSERT_EQ(1, tsp_cln.collector.result->response.can_iface_stats.size());
    ASSERT_EQ(0, tsp_cln.collector.result->response.can_iface_stats[0].errors);
    ASSERT_EQ(1, tsp_cln.collector.result->response.can_iface_stats[0].frames_rx);
    ASSERT_EQ(0, tsp_cln.collector.result->response.can_iface_stats[0].frames_tx);

    /*
     * Second request
     */
    ASSERT_LE(0, tsp_cln.call(1, uavcan::protocol::GetTransportStats::Request()));
    ASSERT_LE(0, nodes.spinBoth(uavcan::MonotonicDuration::fromMSec(10)));

    ASSERT_TRUE(tsp_cln.collector.result.get());
    ASSERT_EQ(0, tsp_cln.collector.result->response.transfer_errors);
    ASSERT_EQ(2, tsp_cln.collector.result->response.transfers_rx);
    ASSERT_EQ(1, tsp_cln.collector.result->response.transfers_tx);
    ASSERT_EQ(1, tsp_cln.collector.result->response.can_iface_stats.size());
    ASSERT_EQ(0, tsp_cln.collector.result->response.can_iface_stats[0].errors);
    ASSERT_EQ(2, tsp_cln.collector.result->response.can_iface_stats[0].frames_rx);
    ASSERT_EQ(8, tsp_cln.collector.result->response.can_iface_stats[0].frames_tx);

    /*
     * Sending a malformed frame, it must be registered as tranfer error
     */
    uavcan::Frame frame(uavcan::protocol::GetTransportStats::DefaultDataTypeID, uavcan::TransferTypeServiceRequest,
                        2, 1, 0, 0, true);
    uavcan::CanFrame can_frame;
    ASSERT_TRUE(frame.compile(can_frame));
    nodes.can_a.read_queue.push(can_frame);

    ASSERT_LE(0, nodes.spinBoth(uavcan::MonotonicDuration::fromMSec(10)));

    /*
     * Introducing a CAN driver error
     */
    nodes.can_a.error_count = 72;

    /*
     * Third request
     */
    ASSERT_LE(0, tsp_cln.call(1, uavcan::protocol::GetTransportStats::Request()));
    ASSERT_LE(0, nodes.spinBoth(uavcan::MonotonicDuration::fromMSec(10)));

    ASSERT_TRUE(tsp_cln.collector.result.get());
    ASSERT_EQ(1, tsp_cln.collector.result->response.transfer_errors);                  // That broken frame
    ASSERT_EQ(3, tsp_cln.collector.result->response.transfers_rx);
    ASSERT_EQ(2, tsp_cln.collector.result->response.transfers_tx);
    ASSERT_EQ(1, tsp_cln.collector.result->response.can_iface_stats.size());
    ASSERT_EQ(72, tsp_cln.collector.result->response.can_iface_stats[0].errors);
    ASSERT_EQ(4, tsp_cln.collector.result->response.can_iface_stats[0].frames_rx);     // Same here
    ASSERT_EQ(16, tsp_cln.collector.result->response.can_iface_stats[0].frames_tx);
}
