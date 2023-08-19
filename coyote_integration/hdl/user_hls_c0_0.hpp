#pragma once

#include <hls_stream.h>
#include <stdint.h>
#include <stdio.h>
#include <ap_int.h>
#include <math.h>

#include "ap_int.h"
#include "stream.hpp"

#define AXI_DATA_BITS       512

#define VADDR_BITS          48
#define LEN_BITS            28
#define DEST_BITS           4
#define PID_BITS            6
#define RID_BITS            4


//
// Structs
//

// AXI stream
struct axisrIntf {
    ap_uint<AXI_DATA_BITS> tdata;
    ap_uint<AXI_DATA_BITS/8> tkeep;
    ap_uint<DEST_BITS> tdest;
    ap_uint<1> tlast;

    axisrIntf()
        : tdata(0), tkeep(0), tdest(0), tlast(0) {}
    axisrIntf(ap_uint<AXI_DATA_BITS> tdata, ap_uint<AXI_DATA_BITS/8> tkeep, ap_uint<DEST_BITS> tdest, ap_uint<1> tlast)
        : tdata(tdata), tkeep(tkeep), tdest(tdest), tlast(tlast) {}
};


//
// User logic top level
//

void design_user_hls_c0_0_top (
    // Host streams
    hls::stream<axisrIntf>& axis_host_sink,
    hls::stream<axisrIntf>& axis_host_src,

    ap_uint<64> numberItems,
    ap_uint<64> windowItems
);