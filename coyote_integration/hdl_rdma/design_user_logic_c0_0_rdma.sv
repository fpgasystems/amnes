`timescale 1ns / 1ps

import lynxTypes::*;

`include "axi_macros.svh"
`include "lynx_macros.svh"

`define CORR

/**
 * User logic
 * 
 */
module design_user_logic_c0_0_rdma (
    // AXI4L CONTROL
    AXI4L.s                     axi_ctrl,

    // DESCRIPTOR BYPASS
    metaIntf.m			        bpss_rd_req,
    metaIntf.m			        bpss_wr_req,
    metaIntf.s                  bpss_rd_done,
    metaIntf.s                  bpss_wr_done,

    // AXI4S HOST STREAMS
    AXI4SR.s                     axis_host_sink,
    AXI4SR.m                     axis_host_src,

    // RDMA QSFP1 CMD
    metaIntf.s			        rdma_1_rd_req,
    metaIntf.s 			        rdma_1_wr_req,

    // AXI4S RDMA QSFP1 STREAMS
    AXI4SR.s                     axis_rdma_1_sink,
    AXI4SR.m                     axis_rdma_1_src,

    // Clock and reset
    input  wire                 aclk,
    input  wire[0:0]            aresetn
);

/* -- Tie-off unused interfaces and signals ----------------------------- */
//always_comb axi_ctrl.tie_off_s();
//always_comb bpss_rd_req.tie_off_m();
//always_comb bpss_wr_req.tie_off_m();
//always_comb bpss_rd_done.tie_off_s();
//always_comb bpss_wr_done.tie_off_s();
//always_comb axis_host_sink.tie_off_s();
//always_comb axis_host_src.tie_off_m();
	
//always_comb rdma_1_rd_req.tie_off_s();
//always_comb rdma_1_wr_req.tie_off_s();
//always_comb axis_rdma_1_sink.tie_off_s();
//always_comb axis_rdma_1_src.tie_off_m();

/* -- USER LOGIC -------------------------------------------------------- */
`ifndef CORR

    `include "perf_rdma_host_c0_0.svh"

`else

`META_ASSIGN(rdma_1_rd_req, bpss_rd_req)
//`META_ASSIGN(rdma_1_wr_req, bpss_wr_req)

//`AXISR_ASSIGN(axis_rdma_1_sink, axis_host_src)
`AXISR_ASSIGN(axis_host_sink, axis_rdma_1_src)

logic [63:0] s_count_tuples;
logic [31:0] s_count_elem;

logic [63:0] s_items;
logic [63:0] s_vaddr;
logic [31:0] s_len;
logic [31:0] s_cachelines;

logic s_data_to_coeff;
logic s_req_sent;

AXI4SR #(.AXI4S_DATA_BITS(512)) axis_corr_data();
AXI4SR #(.AXI4S_DATA_BITS(32))  axis_corr_coeff();
AXI4SR #(.AXI4S_DATA_BITS(512)) axis_rslt_host();

metaIntf #(.STYPE(req_t)) corr_wr_req();

always @(posedge aclk) begin
    if (~aresetn)

        s_count_tuples <= '0;
    else begin

        if (axis_rdma_1_sink.tvalid && axis_rdma_1_sink.tready) begin

            case (axis_rdma_1_sink.tkeep)
                64'hF: s_count_tuples <= s_count_tuples + 64'h1;
                64'hFF: s_count_tuples <= s_count_tuples + 64'h2;
                64'hFFF: s_count_tuples <= s_count_tuples + 64'h3;
                64'hFFFF: s_count_tuples <= s_count_tuples + 64'h4;
                64'hFFFFF: s_count_tuples <= s_count_tuples + 64'h5;
                64'hFFFFFF: s_count_tuples <= s_count_tuples + 64'h6;
                64'hFFFFFFF: s_count_tuples <= s_count_tuples + 64'h7;
                64'hFFFFFFFF: s_count_tuples <= s_count_tuples + 64'h8;
                64'hFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'h9;
                64'hFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hA;
                64'hFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hB;
                64'hFFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hC;
                64'hFFFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hD;
                64'hFFFFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hE;
                64'hFFFFFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'hF;
                64'hFFFFFFFFFFFFFFFF: s_count_tuples <= s_count_tuples + 64'h10;
            endcase
        end

        if(s_count_elem == s_cachelines)
            s_count_tuples <= '0;
    end
end

    design_user_hls_c0_0 inst_user_c0_0 (
	    .s_axi_control_AWVALID(axi_ctrl.awvalid),
        .s_axi_control_AWREADY(axi_ctrl.awready),
        .s_axi_control_AWADDR(axi_ctrl.awaddr),
        .s_axi_control_WVALID(axi_ctrl.wvalid),
        .s_axi_control_WREADY(axi_ctrl.wready),
        .s_axi_control_WDATA(axi_ctrl.wdata),
        .s_axi_control_WSTRB(axi_ctrl.wstrb),
        .s_axi_control_ARVALID(axi_ctrl.arvalid),
        .s_axi_control_ARREADY(axi_ctrl.arready),
        .s_axi_control_ARADDR(axi_ctrl.araddr),
        .s_axi_control_RVALID(axi_ctrl.rvalid),
        .s_axi_control_RREADY(axi_ctrl.rready),
        .s_axi_control_RDATA(axi_ctrl.rdata),
        .s_axi_control_RRESP(axi_ctrl.rresp),
        .s_axi_control_BVALID(axi_ctrl.bvalid),
        .s_axi_control_BREADY(axi_ctrl.bready),
        .s_axi_control_BRESP(axi_ctrl.bresp),
        .s_axis_host_sink_TDATA (axis_corr_data.tdata),
        .s_axis_host_sink_TKEEP (axis_corr_data.tkeep),
        .s_axis_host_sink_TLAST (axis_corr_data.tlast),
        .s_axis_host_sink_TVALID(axis_corr_data.tvalid),
        .s_axis_host_sink_TREADY(axis_corr_data.tready),
        .s_axis_host_sink_TDEST ('b0),
        .m_axis_host_src_TDATA  (axis_corr_coeff.tdata),
        .m_axis_host_src_TKEEP  (axis_corr_coeff.tkeep),
        .m_axis_host_src_TLAST  (axis_corr_coeff.tlast),
        .m_axis_host_src_TVALID (axis_corr_coeff.tvalid),
        .m_axis_host_src_TREADY (axis_corr_coeff.tready),
        .m_axis_host_src_TDEST(),
        .itemsReg(s_items),
        .vaddrReg(s_vaddr), 
        .lenReg  (s_len),  
        .pidReg  (s_cachelines),  
        .ap_clk(aclk),
        .ap_rst_n(aresetn)
	);

    axis_coeff_host axis_coeff_host_inst(
        .s_axis_tdata  (axis_corr_coeff.tdata[31:0]),
        .s_axis_tkeep  (axis_corr_coeff.tkeep[3:0]),
        .s_axis_tlast  (axis_corr_coeff.tlast),
        .s_axis_tvalid (axis_corr_coeff.tvalid),
        .s_axis_tready (axis_corr_coeff.tready),
        .m_axis_tdata  (axis_rslt_host.tdata),
        .m_axis_tkeep  (axis_rslt_host.tkeep),
        .m_axis_tlast  (axis_rslt_host.tlast),
        .m_axis_tvalid (axis_rslt_host.tvalid),
        .m_axis_tready (axis_rslt_host.tready), 
        .aclk(aclk),
        .aresetn(aresetn)
	);

    ila_rdma ila_corr_inst(
    .clk(aclk),
    .probe0(axis_rdma_1_sink.tlast),
    .probe1(axis_rdma_1_sink.tready),
    .probe2(axis_rdma_1_sink.tvalid),
    .probe3(axis_corr_data.tlast),
    .probe4(axis_corr_data.tready),
    .probe5(axis_corr_data.tvalid),
    .probe6(bpss_wr_done.valid),
    .probe7(axis_rslt_host.tvalid),
    .probe8(axis_rslt_host.tready),
    .probe9 (axis_corr_coeff.tlast),
    .probe10(axis_corr_coeff.tready),
    .probe11(axis_corr_coeff.tvalid),
    .probe12(s_data_to_coeff),
    .probe13(bpss_wr_req.ready),
    .probe14(bpss_wr_req.valid),
    .probe15(bpss_wr_req.data.ctl),
    .probe16(axis_corr_data.tdata[63:32]),
    .probe17(axis_rslt_host.tdata[63:0]),
    .probe18(bpss_wr_req.data.len),
    .probe19(bpss_wr_req.data.vaddr),
    .probe20(axis_rdma_1_sink.tdata[31:0]),
    .probe21(corr_wr_req.data.vaddr),
    .probe22(axis_corr_coeff.tdata[31:0]),
    .probe23(axis_corr_data.tdata[31:0])
    );
	
	ila_rdma ila_logic_inst(
    .clk(aclk),
    .probe0(axi_ctrl.arready),
    .probe1(rdma_1_rd_req.valid),
    .probe2(axi_ctrl.rready),
    .probe3(s_req_sent),
    .probe4(corr_wr_req.ready),
    .probe5(axi_ctrl.arvalid),
    .probe6(axi_ctrl.awvalid),
    .probe7(axi_ctrl.wvalid),
    .probe8(axi_ctrl.awready),
    .probe9(bpss_wr_req.ready),
    .probe10(axi_ctrl.wready),
    .probe11(bpss_wr_req.valid),
    .probe12(s_data_to_coeff),
    .probe13(corr_wr_req.valid),
    .probe14(bpss_wr_done.valid),
    .probe15(axi_ctrl.rdata), 
    .probe16(s_items),
    .probe17(s_vaddr),
    .probe18(s_len),
    .probe19(axi_ctrl.araddr),
    .probe20(s_count_elem),
    .probe21(s_count_tuples),
    .probe22(axi_ctrl.awaddr),
    .probe23(axi_ctrl.wdata)
    );

    // assign s_data_to_coeff = (s_count_tuples == 'h4000 &&) ? 1'b1 : 1'b0; //(s_items!=0 && s_count_tuples == 'h4000) ? 1'b1 : 1'b0; //(s_items<<4)
    
    // axis_rdma_1_sink -> axis_corr_data ->|
    //                                      | -> axis_host_src
    //                     axis_corr_rslt ->|
    assign axis_corr_data.tdata  = axis_rdma_1_sink.tdata;    
    assign axis_corr_data.tvalid = axis_rdma_1_sink.tvalid;
    assign axis_corr_data.tkeep  = axis_rdma_1_sink.tkeep; 
    assign axis_corr_data.tlast  = axis_rdma_1_sink.tlast;
    
    assign axis_rdma_1_sink.tready = axis_corr_data.tready & axis_host_src.tready;

    assign axis_host_src.tdata  = s_data_to_coeff ? axis_rslt_host.tdata  : axis_corr_data.tdata;
    assign axis_host_src.tvalid = s_data_to_coeff ? axis_rslt_host.tvalid : axis_corr_data.tvalid;
    assign axis_host_src.tkeep  = s_data_to_coeff ? axis_rslt_host.tkeep  : axis_corr_data.tkeep;
    assign axis_host_src.tlast  = s_data_to_coeff ? axis_rslt_host.tlast  : axis_corr_data.tlast;
        
    assign axis_rslt_host.tready = axis_host_src.tready;

    // rdma_1_wr_req ->|
    //                 | -> bpss_wr_req
    // corr_wr_req   ->|
    assign bpss_wr_req.data  = s_data_to_coeff ? corr_wr_req.data  : rdma_1_wr_req.data;
    assign bpss_wr_req.valid = s_data_to_coeff ? corr_wr_req.valid : rdma_1_wr_req.valid; 

    assign rdma_1_wr_req.ready = bpss_wr_req.ready;
    assign corr_wr_req.ready   = bpss_wr_req.ready;
    
    //assign corr_wr_req.data.len    = s_len[11:0];
    //assign corr_wr_req.data.pid    = s_pid[5:0];
    //assign corr_wr_req.data.vaddr  = s_vaddr[15:0];    
    //assign corr_wr_req.data.host   = 1'b1;
    //assign corr_wr_req.data.ctl    = 1'b1;
    //assign corr_wr_req.data.stream = 1'b1;
    //assign corr_wr_req.data.sync   = 1'b0; 

    assign corr_wr_req.data.len    = s_len[11:0];
    assign corr_wr_req.data.pid    = rdma_1_wr_req.data.pid;
    assign corr_wr_req.data.vaddr  = rdma_1_wr_req.data.vaddr+rdma_1_wr_req.data.len;    
    assign corr_wr_req.data.host   = rdma_1_wr_req.data.host;
    assign corr_wr_req.data.ctl    = rdma_1_wr_req.data.ctl;
    assign corr_wr_req.data.stream = rdma_1_wr_req.data.stream;
    assign corr_wr_req.data.sync   = rdma_1_wr_req.data.sync; 

    always @(posedge aclk) begin
        if (~aresetn) begin

            corr_wr_req.valid <= 1'b0;
            s_req_sent <= 1'b0;  
            
        end 
        else begin

            corr_wr_req.valid       <= 1'b0;

            if(s_data_to_coeff && corr_wr_req.ready && !s_req_sent) begin    

                corr_wr_req.valid       <= 1'b1;
                s_req_sent              <= 1'b1;
            end

            if(s_req_sent && s_count_elem == s_cachelines)
                s_req_sent <= 1'b0;

        end  
    end
    
    always @(posedge aclk) begin
        if (~aresetn) begin

            s_count_elem <= '0; 
            s_data_to_coeff <= 1'b0;
        end     
        else begin
                       
            if(s_items!=0 && s_count_tuples == (s_items<<4))
                s_data_to_coeff <= 1'b1;    

            if(s_data_to_coeff && axis_rslt_host.tvalid && axis_rslt_host.tready)
                s_count_elem <= s_count_elem+1;

            if(s_count_elem == s_cachelines) begin

                s_data_to_coeff <= 1'b0;
                s_count_elem <= '0;
            end
        end  
    end

`endif

endmodule