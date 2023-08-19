`timescale 1ns / 1ps
	 
import lynxTypes::*;

`include "axi_macros.svh"
`include "lynx_macros.svh"
	
/**
 * User logic wrapper
 * 
 */
module design_user_wrapper_0 (
    // AXI4L CONTROL
    input  logic[AXI_ADDR_BITS-1:0]             axi_ctrl_araddr,
    input  logic[2:0]                           axi_ctrl_arprot,
    output logic                                axi_ctrl_arready,
    input  logic                                axi_ctrl_arvalid,
    input  logic[AXI_ADDR_BITS-1:0]             axi_ctrl_awaddr,
    input  logic[2:0]                           axi_ctrl_awprot,
    output logic                                axi_ctrl_awready,
    input  logic                                axi_ctrl_awvalid, 
    input  logic                                axi_ctrl_bready,
    output logic[1:0]                           axi_ctrl_bresp,
    output logic                                axi_ctrl_bvalid,
    output logic[AXI_ADDR_BITS-1:0]             axi_ctrl_rdata,
    input  logic                                axi_ctrl_rready,
    output logic[1:0]                           axi_ctrl_rresp,
    output logic                                axi_ctrl_rvalid,
    input  logic[AXIL_DATA_BITS-1:0]            axi_ctrl_wdata,
    output logic                                axi_ctrl_wready,
    input  logic[(AXIL_DATA_BITS/8)-1:0]        axi_ctrl_wstrb,
    input  logic                                axi_ctrl_wvalid,
	
    // DESCRIPTOR BYPASS
    output logic 							    bpss_rd_req_valid,
    input  logic 							    bpss_rd_req_ready,
    output req_t 							    bpss_rd_req_data,
    output logic 							    bpss_wr_req_valid,
    input  logic 							    bpss_wr_req_ready,
    output req_t 							    bpss_wr_req_data,
    input  logic                                bpss_rd_done_valid,
    output logic                                bpss_rd_done_ready,
    input  logic [PID_BITS-1:0]                 bpss_rd_done_data,
    input  logic                                bpss_wr_done_valid,
    output logic                                bpss_wr_done_ready,
    input  logic [PID_BITS-1:0]                 bpss_wr_done_data,
		
    // AXI4S HOST SINK
    input  logic[AXI_DATA_BITS-1:0]             axis_host_sink_tdata,
    input  logic[AXI_DATA_BITS/8-1:0]           axis_host_sink_tkeep,
    input  logic[PID_BITS-1:0]                  axis_host_sink_tid,
    input  logic                                axis_host_sink_tlast,
    output logic                                axis_host_sink_tready,
    input  logic                                axis_host_sink_tvalid,

	// AXI4S HOST SOURCE
    output logic[AXI_DATA_BITS-1:0]             axis_host_src_tdata,
    output logic[AXI_DATA_BITS/8-1:0]           axis_host_src_tkeep,
    output logic[PID_BITS-1:0]                  axis_host_src_tid,
    output logic                                axis_host_src_tlast,
    input  logic                                axis_host_src_tready,
    output logic                                axis_host_src_tvalid,
        
    // RDMA QSFP1 CMD
    input  logic 							    rdma_1_rd_req_valid,
    output logic 							    rdma_1_rd_req_ready,
    input  req_t 							    rdma_1_rd_req_data,
    input  logic 							    rdma_1_wr_req_valid,
    output logic 							    rdma_1_wr_req_ready,
    input  req_t 							    rdma_1_wr_req_data,


    // AXI4S RDMA QSFP1 SINK
    input  logic                                axis_rdma_1_sink_tlast,
    output logic                                axis_rdma_1_sink_tready,
    input  logic                                axis_rdma_1_sink_tvalid,
    input  logic[AXI_NET_BITS-1:0]		        axis_rdma_1_sink_tdata,
    input  logic[AXI_NET_BITS/8-1:0]	        axis_rdma_1_sink_tkeep,
    input  logic[PID_BITS-1:0]	                axis_rdma_1_sink_tid,

    // AXI4S RDMA QSFP1 SOURCE
    output logic                                axis_rdma_1_src_tlast,
    input  logic                                axis_rdma_1_src_tready,
    output logic                                axis_rdma_1_src_tvalid,
    output logic[AXI_NET_BITS-1:0]		        axis_rdma_1_src_tdata,
    output logic[AXI_NET_BITS/8-1:0]	        axis_rdma_1_src_tkeep,
    output logic[PID_BITS-1:0]	                axis_rdma_1_src_tid,

    // Clock and reset
    input  logic                                aclk,
    input  logic[0:0]                           aresetn,

    // BSCAN
    input  logic                                S_BSCAN_drck,
    input  logic                                S_BSCAN_shift,
    input  logic                                S_BSCAN_tdi,
    input  logic                                S_BSCAN_update,
    input  logic                                S_BSCAN_sel,
    output logic                                S_BSCAN_tdo,
    input  logic                                S_BSCAN_tms,
    input  logic                                S_BSCAN_tck,
    input  logic                                S_BSCAN_runtest,
    input  logic                                S_BSCAN_reset,
    input  logic                                S_BSCAN_capture,
    input  logic                                S_BSCAN_bscanid_en
);
	
    // Control
    AXI4L axi_ctrl_user();

    assign axi_ctrl_user.araddr                 = axi_ctrl_araddr;
    assign axi_ctrl_user.arprot                 = axi_ctrl_arprot;
    assign axi_ctrl_user.arvalid                = axi_ctrl_arvalid;
    assign axi_ctrl_user.awaddr                 = axi_ctrl_awaddr;
    assign axi_ctrl_user.awprot                 = axi_ctrl_awprot;
    assign axi_ctrl_user.awvalid                = axi_ctrl_awvalid;
    assign axi_ctrl_user.bready                 = axi_ctrl_bready;
    assign axi_ctrl_user.rready                 = axi_ctrl_rready;
    assign axi_ctrl_user.wdata                  = axi_ctrl_wdata;
    assign axi_ctrl_user.wstrb                  = axi_ctrl_wstrb;
    assign axi_ctrl_user.wvalid                 = axi_ctrl_wvalid;
    assign axi_ctrl_arready                     = axi_ctrl_user.arready;
    assign axi_ctrl_awready                     = axi_ctrl_user.awready;
    assign axi_ctrl_bresp                       = axi_ctrl_user.bresp;
    assign axi_ctrl_bvalid                      = axi_ctrl_user.bvalid;
    assign axi_ctrl_rdata                       = axi_ctrl_user.rdata;
    assign axi_ctrl_rresp                       = axi_ctrl_user.rresp;
    assign axi_ctrl_rvalid                      = axi_ctrl_user.rvalid;
    assign axi_ctrl_wready                      = axi_ctrl_user.wready;
	
    // Descriptor bypass
    metaIntf #(.STYPE(req_t)) bpss_rd_req();
    metaIntf #(.STYPE(req_t)) bpss_wr_req();

    assign bpss_rd_req_valid                    = bpss_rd_req.valid;
    assign bpss_rd_req.ready                    = bpss_rd_req_ready;
    assign bpss_rd_req_data                     = bpss_rd_req.data;
    assign bpss_wr_req_valid                    = bpss_wr_req.valid;
    assign bpss_wr_req.ready                    = bpss_wr_req_ready;
    assign bpss_wr_req_data                     = bpss_wr_req.data;

    metaIntf #(.STYPE(logic[PID_BITS-1:0])) bpss_rd_done();
    metaIntf #(.STYPE(logic[PID_BITS-1:0])) bpss_wr_done();

    assign bpss_rd_done.valid                   = bpss_rd_done_valid;
    assign bpss_rd_done_ready                   = 1'b1;
    assign bpss_rd_done.data                    = bpss_rd_done_data;
    assign bpss_wr_done.valid                   = bpss_wr_done_valid;
    assign bpss_wr_done_ready                   = 1'b1;
    assign bpss_wr_done.data                    = bpss_wr_done_data;
		
    // AXIS host sink
    AXI4SR axis_host_sink();
    
    assign axis_host_sink.tdata                 = axis_host_sink_tdata;
    assign axis_host_sink.tkeep                 = axis_host_sink_tkeep;
    assign axis_host_sink.tid                   = axis_host_sink_tid;
    assign axis_host_sink.tlast                 = axis_host_sink_tlast;
    assign axis_host_sink.tvalid                = axis_host_sink_tvalid;
    assign axis_host_sink_tready                = axis_host_sink.tready;

    // AXIS host source
    AXI4SR axis_host_src();
    
    assign axis_host_src_tdata                  = axis_host_src.tdata;
    assign axis_host_src_tkeep                  = axis_host_src.tkeep;
    assign axis_host_src_tid                    = axis_host_src.tid;
    assign axis_host_src_tlast                  = axis_host_src.tlast;
    assign axis_host_src_tvalid                 = axis_host_src.tvalid;
    assign axis_host_src.tready                 = axis_host_src_tready;
        
    // RDMA commands
    metaIntf #(.STYPE(req_t)) rdma_1_rd_req();
    metaIntf #(.STYPE(req_t)) rdma_1_wr_req();
    metaIntf #(.STYPE(req_t)) rdma_1_wr_req_mux();

    assign rdma_1_rd_req.valid                  = rdma_1_rd_req_valid;
    assign rdma_1_rd_req_ready                  = rdma_1_rd_req.ready;
    assign rdma_1_rd_req.data                   = rdma_1_rd_req_data;
    assign rdma_1_wr_req.valid                  = rdma_1_wr_req_valid;
    assign rdma_1_wr_req_ready                  = rdma_1_wr_req.ready;
    assign rdma_1_wr_req.data                   = rdma_1_wr_req_data;

    // AXIS RDMA sink
    AXI4SR axis_rdma_1_sink();
    AXI4SR axis_rdma_1_sink_mux();

    assign axis_rdma_1_sink.tdata               = axis_rdma_1_sink_tdata;
    assign axis_rdma_1_sink.tkeep               = axis_rdma_1_sink_tkeep;
    assign axis_rdma_1_sink.tid                 = axis_rdma_1_sink_tid;
    assign axis_rdma_1_sink.tlast               = axis_rdma_1_sink_tlast;
    assign axis_rdma_1_sink.tvalid              = axis_rdma_1_sink_tvalid;
    assign axis_rdma_1_sink_tready              = axis_rdma_1_sink.tready;

    // AXIS RDMA source
    AXI4SR axis_rdma_1_src();

    assign axis_rdma_1_src_tdata                = axis_rdma_1_src.tdata;
    assign axis_rdma_1_src_tkeep                = axis_rdma_1_src.tkeep;
    assign axis_rdma_1_src_tid                  = axis_rdma_1_src.tid;
    assign axis_rdma_1_src_tlast                = axis_rdma_1_src.tlast;
    assign axis_rdma_1_src_tvalid               = axis_rdma_1_src.tvalid;
    assign axis_rdma_1_src.tready               = axis_rdma_1_src_tready;

    `META_ASSIGN(rdma_1_wr_req, rdma_1_wr_req_mux)
    `AXISR_ASSIGN(axis_rdma_1_sink, axis_rdma_1_sink_mux)
	
    //
	// USER LOGIC
    //

     design_user_logic_c0_0_rdma inst_user_c0_0_rdma (
        .axi_ctrl(axi_ctrl_user),
        .bpss_rd_req(bpss_rd_req),
        .bpss_wr_req(bpss_wr_req),
        .bpss_rd_done(bpss_rd_done),
        .bpss_wr_done(bpss_wr_done),
        .axis_host_sink(axis_host_sink),
        .axis_host_src(axis_host_src),
        .rdma_1_rd_req(rdma_1_rd_req),
        .rdma_1_wr_req(rdma_1_wr_req_mux),
        .axis_rdma_1_src(axis_rdma_1_src),
        .axis_rdma_1_sink(axis_rdma_1_sink_mux),
        .aclk(aclk),
        .aresetn(aresetn)
    );

    ila_rdma ila_rdma_inst(
        .clk(aclk),
        .probe0(axis_rdma_1_src.tlast),
        .probe1(axis_rdma_1_src.tready),
        .probe2(axis_rdma_1_src.tvalid),
        .probe3(axis_rdma_1_sink_mux.tlast),
        .probe4(axis_rdma_1_sink_mux.tready),
        .probe5(axis_rdma_1_sink_mux.tvalid),
        .probe6(axis_host_src.tvalid),
        .probe7(axis_host_src.tlast),
        .probe8(axis_host_src.tready),
        .probe9(axis_host_sink.tlast),
        .probe10(axis_host_sink.tready),
        .probe11(axis_host_sink.tvalid),
        .probe12(bpss_rd_req.valid),
        .probe13(bpss_rd_req.ready),
        .probe14(bpss_wr_req.valid),
        .probe15(bpss_wr_req.ready),
        .probe16(bpss_wr_done.valid),
        .probe17(bpss_rd_done.valid),
        .probe18(axis_rdma_1_src.tdata[31:0]),
        .probe19(axis_rdma_1_sink_mux.tdata[31:0]),
        .probe20(axis_host_src.tdata[31:0]),
        .probe21(axis_host_sink.tdata[31:0]),
        .probe22(bpss_wr_req.data[31:0]),
        .probe23(rdma_1_wr_req_data)
    );

endmodule	