`timescale 1ns/1ps

module testbench_corr();

localparam real CLK_PERIOD = 3.2;

localparam logic[31:0] ADDR_ELEM  = 32'h10;
localparam logic[31:0] ELEMENTS   = 32'h0A; //32'h3E8; //32'h64;
localparam logic[31:0]  DATA      = 32'h00000000;
localparam logic[511:0] CACHLINE  = '0;

logic reset_n;
logic clk;

logic s_config;
logic s_start;
logic s_second;
logic s_second_data;

logic [31:0] s_cntr_elements;
logic [31:0] s_cntr_results;
logic [31:0] s_data;

logic[511:0] s_sink_data;
logic[63:0]  s_sink_keep; 
logic[3:0]   s_sink_dest;
logic[0:0]   s_sink_last; 
logic        s_sink_valid; 
logic        s_sink_ready;

logic[511:0] s_src_data;
logic[63:0]  s_src_keep; 
logic[3:0]   s_src_dest;
logic[0:0]   s_src_last; 
logic        s_src_valid; 
logic        s_src_ready;

logic       s_ctrl_awvalid;
logic       s_ctrl_awready;
logic[4:0]  s_ctrl_awaddr;
logic       s_ctrl_wvalid;
logic       s_ctrl_wready;
logic[31:0] s_ctrl_wdata;
logic[3:0]  s_ctrl_wstrb;

clock_gen inst_clock_gen (
  .clk_period(CLK_PERIOD),
  .clk(clk)    
);

initial 
begin 
    reset_n = 0; #50ns;
    reset_n = 1; //#20us;
end // initial


design_user_hls_c0_0 inst_design_user_hls_c0_0_top(
    .ap_clk (clk),
    .ap_rst_n (reset_n),
    .s_axi_control_AWVALID (s_ctrl_awvalid),
    .s_axi_control_AWREADY (s_ctrl_awready),
    .s_axi_control_AWADDR  (ADDR_ELEM), 
    .s_axi_control_WVALID  (s_ctrl_wvalid),
    .s_axi_control_WREADY  (s_ctrl_wready),
    .s_axi_control_WDATA   (s_ctrl_wdata),
    .s_axi_control_WSTRB   (s_ctrl_wstrb),
    .s_axi_control_ARVALID (1'b0),
    .s_axi_control_ARREADY (),
    .s_axi_control_ARADDR  ('h0),
    .s_axi_control_RVALID  (1'b1),
    .s_axi_control_RREADY  (),
    .s_axi_control_RDATA   (),
    .s_axi_control_RRESP   (),
    .s_axi_control_BVALID  (),
    .s_axi_control_BREADY  (1'b1),
    .s_axi_control_BRESP   (),
    // sink 
    .s_axis_host_sink_TDATA  (s_sink_data),
    .s_axis_host_sink_TKEEP  (s_sink_keep),
    .s_axis_host_sink_TSTRB  ('h1),
    .s_axis_host_sink_TDEST  (s_sink_dest),
    .s_axis_host_sink_TLAST  (s_sink_last),   
    .s_axis_host_sink_TVALID (s_sink_valid),
    .s_axis_host_sink_TREADY (s_sink_ready),
    // source
    .m_axis_host_src_TDATA   (s_src_data),
    .m_axis_host_src_TKEEP   (s_src_keep),
    .m_axis_host_src_TDEST   (s_src_dest),
    .m_axis_host_src_TSTRB   (),
    .m_axis_host_src_TLAST   (s_src_last),
    .m_axis_host_src_TVALID  (s_src_valid),
    .m_axis_host_src_TREADY  (s_src_ready));
    
//process for control
always @(posedge clk) begin
    if (~reset_n) begin

        s_ctrl_awvalid <= 1'b0;
        s_ctrl_wvalid  <= 1'b0;
        s_ctrl_wdata   <= '0;
        s_ctrl_wstrb   <= '0;

        s_config       <= 1'b1;
        s_second       <= 1'b0;
    end
    else begin

        s_ctrl_awvalid <= 1'b0;
        s_ctrl_wvalid  <= 1'b0;   
        
        if (s_ctrl_awready && s_config)
            s_ctrl_awvalid <= 1'b1;
        
        if (s_ctrl_wready && s_config) begin

            s_ctrl_wdata  <= ELEMENTS;
            s_ctrl_wvalid <= 1'b1;
            s_ctrl_wstrb  <= '1;
            
            s_config <= 1'b0;
        end

        if (s_src_last && (~s_second)) begin

            s_ctrl_wstrb  <= '0;
            s_ctrl_wdata  <= '0;

            s_config <= 1'b1;
            s_second <= 1'b1;
        end
    end
end

//process for sink
always @(posedge clk) begin
    if (~reset_n) begin

        s_sink_data <= '0;
        s_sink_keep <= '0;
        s_sink_dest <= '0;
        s_sink_last <= '0;
        s_sink_valid<= 1'b0;

        s_src_ready     <= 1'b0;

        s_data          <= DATA;
        s_cntr_elements <= '0;

        s_start        <= 1'b0;
        s_second_data  <= 1'b0;
    end
    else begin

        s_sink_valid <= 1'b0;

        if ((~s_config) && (~s_start)) begin

            s_sink_data  <= {s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data};
            s_sink_valid <= 1'b1;

            s_data          <= s_data + 1;
            s_cntr_elements <= s_cntr_elements + 1;
            s_start         <= 1'b1;
        end
        
        if (s_start && s_sink_ready && s_cntr_elements!=ELEMENTS) begin

            s_sink_data  <= {s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data,s_data};
            s_sink_valid <= 1'b1;

            s_data          <= s_data + 1;
            s_cntr_elements <= s_cntr_elements + 1;

        end

        if (s_second && (~s_second_data)) begin

           s_start         <= 1'b0;
           s_cntr_elements <= '0;    
           s_data          <= DATA; 

           s_second_data   <= 1'b1;
        end
    end
end

//process for src
always @(posedge clk) begin
    if (~reset_n) begin
        s_src_ready    <= 1'b0;
        s_cntr_results <= '0;
    end
    else begin
        s_src_ready <= 1'b1;

        if (s_src_valid) begin
           integer v_coef_rang = integer(s_cntr_results);
           shortreal v_coef_value = shortreal(s_src_data[31:0]);
           
           $display("Coeff %d : %2.2f \n", v_coef_rang, v_coef_value);
           s_cntr_results <= s_cntr_results + 1;
        end
        
    end
end

endmodule // testbench_corr