`timescale 1ns/1ps

module clock_gen #(parameter INIT_DELAY=0)(
   input  real   clk_period,
   output logic  clk
);

   always begin
     #(clk_period/2.0) clk = 0;
     #(clk_period/2.0) clk = 1;
   end
endmodule
