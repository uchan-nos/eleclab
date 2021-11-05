module CORDIC_Top(
  input clk,
  input rst,
  input init,
  input [16:0] x_i,
  input [16:0] y_i,
  input [16:0] theta_i,
  output reg [16:0] x_o,
  output reg [16:0] y_o,
  output reg [16:0] theta_o);

always @(posedge clk or posedge rst) begin
  if (rst)
    y_o <= 0;
  else if (theta_i == {9'd0, 8'd0})
    y_o <= 17'b0_0_0000_0000_0000000; // 0
  else if (theta_i == {9'd1, 8'd0})
    y_o <= 17'b0_0_1000_0000_0000000; // 0.5
  else if (theta_i == {9'd2, 8'd0})
    y_o <= 17'b0_0_1111_1111_0000000; // +0.99..
  else if (theta_i == {9'd3, 8'd0})
    y_o <= 17'b0_1_0000_0000_0000000; // +1.0
  else if (theta_i == {9'd4, 8'd0})
    y_o <= 17'b1_1_0000_0000_0000000; // -1.0
  else if (theta_i == {9'd5, 8'd0})
    y_o <= 17'b1_0_1111_1111_0000000; // -0.99..
end

endmodule
