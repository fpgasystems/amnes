#ifndef CORRELATION_HPP
#define CORRELATION_HPP

#include "cm_operations.hpp"
#include "../axi_utils.hpp"
#include "../memory/mem_utils.hpp"
#include "../globals.hpp"

void correlation(
    hls::stream<net_axis<LINEW> >& s_axis_data,
    hls::stream<net_axis<MEMW> >&  m_axis_write_data,
    ap_uint<REGW>                  regSampleSize,
    ap_uint<REGW>                  regBaseAddr);

#endif