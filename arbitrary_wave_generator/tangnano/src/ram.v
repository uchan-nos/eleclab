module AutoAddrRam(
  input clk,
  input rst,
  input wr, // 1 = write, 0 = read
  input [8:0] data_in,
  output [8:0] data_out,
  output reg of // 1 = addr is overflowed
);

/*
https://rawgit.com/osamutake/tchart-coffee/master/bin/editor-offline.html

clk       _____~_~_~_~_~_~_~_~_~_
rst       ~~_____________________
wr        ___~~~~~~______________
data_in   ==?=X0=X1=X2=X=?============
data_out  =?==========X?=X0=X1=X2=X0=X1=
addr      =0====X1=X2=X3=X0=X1=X2=X0=X1=X2=
addr_last =0======X1=X2=============
*/

reg [10:0] addr;
reg [10:0] addr_last;
reg wr_posedge;

always @(negedge clk or posedge rst) begin
  if (rst) begin
    of <= 1'd0;
    addr <= 11'd0;
    addr_last <= 11'd0;
  end
  else if (wr_posedge) begin
    {of, addr} <= addr + 11'd1;
    addr_last <= addr;
  end
  else if (addr < addr_last) begin
    {of, addr} <= addr + 11'd1;
  end
  else begin
    of <= 1'd0;
    addr <= 11'd0;
  end
end

always @(posedge clk or posedge rst) begin
  if (rst)
    wr_posedge <= 1'd0;
  else
    wr_posedge <= wr;
end

Gowin_SP bram1(
  .dout(data_out), //output [7:0] dout
  .clk(clk), //input clk
  .oce(1'd0), //input oce
  .ce(1'd1), //input ce
  .reset(rst), //input reset
  .wre(wr), //input wre
  .ad(addr), //input [10:0] ad
  .din(data_in) //input [7:0] din
);

endmodule
