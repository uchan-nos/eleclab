module Initializer(
  input clk,
  input rst_n,
  output reg [BITWIDTH-1:0] data,
  output reg data_v,
  output reg initialized
);

/*
 * サイン波を計算して data に出力する。
 *
 * data: 11..1 = 1.0
 *       10..0 = 0.5
 *       00..0 = 0.0
 *
 * data_v: data の準備ができたら 1
 * initialized: 初期化が完了したら 1
 */

parameter BITWIDTH = 9;

reg [8:0] angle_deg_lin;
reg signed [8:0] cor_angle_deg;
wire [16:0] cor_in;

assign cor_in = {cor_angle_deg[8:0], 8'd0};

reg cor_iter, cor_init, cor_outv;
reg [3:0] cor_cnt; // 0 - 15

wire [16:0] cor_out;

wire [7:0] uart_data;
wire uart_recved;

always @(posedge clk or negedge rst_n) begin
  if (!rst_n || initialized)
    cor_iter <= 1'd0;
  else if (!cor_iter && !cor_init && cor_cnt == 4'd0)
    cor_iter <= 1'd1;
  else
    cor_iter <= 1'd0;
end

always @(posedge clk or negedge rst_n) begin
  if (!rst_n)
    cor_init <= 1'd0;
  else
    cor_init <= cor_iter;
end

always @(posedge cor_iter or negedge rst_n) begin
  if (!rst_n)
    angle_deg_lin <= 9'd0;
  else
    angle_deg_lin <= angle_deg_lin + 9'd1;
end

always @(posedge clk or negedge rst_n) begin
  if (!rst_n)
    initialized <= 1'd0;
  else if (angle_deg_lin >= 9'd360)
    initialized <= 1'd1;
end

always @(posedge cor_init or negedge rst_n) begin
  if (!rst_n) begin
    cor_angle_deg <= 9'd0;
  end
  else if (angle_deg_lin < 9'd90) begin
    cor_angle_deg <= angle_deg_lin;
  end
  else if (angle_deg_lin < 9'd180) begin
    cor_angle_deg <= 9'd180 - angle_deg_lin;
  end
  else if (angle_deg_lin < 9'd270) begin
    cor_angle_deg <= 9'd180 - angle_deg_lin;
  end
  else begin
    cor_angle_deg <= angle_deg_lin - 9'd360;
  end
end

always @(posedge clk or negedge rst_n) begin
  if (!rst_n)
    cor_cnt <= 4'd0;
  else if (cor_cnt > 0)
    cor_cnt <= cor_cnt + 4'd1;
  else if (cor_init)
    cor_cnt <= 4'd1;
end

always @(posedge clk or negedge rst_n) begin
  if (!rst_n)
    cor_outv <= 1'd0;
  else if (cor_cnt == 4'd15)
    cor_outv <= 1'd1;
  else
    cor_outv <= 1'd0;
end

always @(posedge clk or negedge rst_n) begin
  if (!rst_n)
    data_v <= 1'd0;
  else
    data_v <= cor_outv;
end

always @(posedge data_v or negedge rst_n) begin
  if (!rst_n) begin
    data <= 9'd0;
  end
  else begin
    if (cor_out[16]) begin // minus
      if (cor_out[15:7] <= 9'h100) // sin <= -1
        data <= 9'd0;
      else
        data <= 9'd255 + {cor_out[16], cor_out[14:7]};
    end
    else begin // plus
      if (cor_out[15:7] >= 9'h100) // sin >= +1
        data <= 9'd511;
      else
        data <= 9'd255 + {cor_out[16], cor_out[14:7]};
    end
  end
end

/*
*
* 2's complementary fixed-point value.
*
* 0.999... == 8'b0111_1111
* 127 + 0.999... == 8'b0111_1111 + 8'b0111_1111
*                == 8'b1111_1110
*
* -0.999... == 8'b1000_0001
* 127 - 0.999... == 127 + (-0.999...)
*                == 8'b0111_1111 + 8'b1000_0001
*                == 8'b0000_0000
*
*  1.00000001 == 10'b01_0000_0001
* -1.00000001 == 10'b10_1111_1111
*/

CORDIC_Top cordic(
  .clk(clk), //input clk
  .rst(!rst_n), //input rst
  .init(cor_init), //input init
  .x_i(17'd19898), //input [16:0] x_i
  .y_i(17'd0), //input [16:0] y_i
  .theta_i(cor_in), //input [16:0] theta_i
  .x_o(), //output [16:0] x_o
  .y_o(cor_out), //output [16:0] y_o
  .theta_o() //output [16:0] theta_o
);

endmodule
