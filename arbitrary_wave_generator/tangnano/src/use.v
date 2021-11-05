module User(
  input sys_clk,
  input rst_n_raw,
  output [8:0] data,
  output [2:0] led
);

/*
https://rawgit.com/osamutake/tchart-coffee/master/bin/editor-offline.html

sys_clk  _~_~_~_~_~_~:…:_~_~_~_~_~_~_~_
rst_n    ~~__________:…:_______________
cor_iter ___~~_______:…:_______~~______
cor_init _____~~_____:…:_________~~____
cor_in   _____==_____:…:_________==____
cor_cnt  =0======X1=X2=X:…:=X14=X15=X0=====X1=X2=
cor_out  X?===========:…:=?====Xsin=X?=======
cor_outv ____________:…:_____~~________
data     =?===========:…:=?======Xdat=X?=====
data_v   ____________:…:_______~~______
clk_w    ____________:…:_________~~____
*/

// スイッチのチャタリング対策
reg rst_n;
always @(posedge sys_clk) begin
  rst_n <= rst_n_raw;
end

reg clk_w; // 波形を書き込むクロック
reg wave_clk; // 波形を読み出すクロック
wire initialized;
wire clk_r, osc_clk, pll_clk;
wire ram_clk; // RAM に供給するクロック
wire data_v; // ran_din が有効であることを示すフラグ

wire [8:0] ram_din;
wire [8:0] ram_dout;
assign data = initialized ? ram_dout : ram_din;

assign clk_r = initialized & wave_clk;
assign ram_clk = clk_w | clk_r;

assign led[0] = clk_r;
assign led[1] = 1'b1;
assign led[2] = ~initialized;

Gowin_OSC osc(
  .oscout(osc_clk) // 120MHz
);

Gowin_rPLL pll(
  .clkout(pll_clk), // 50MHz
  .clkin(osc_clk)
);

reg [31:0] divider;
always @(posedge pll_clk or negedge rst_n) begin
  if (!rst_n) begin
    divider <= 32'd0;
    wave_clk <= 1'd0;
  end
  else if (divider <= 32'd50000)
    divider <= divider + 32'd1;
  else begin
    divider <= 32'd0;
    wave_clk <= ~wave_clk;
  end
end

AutoAddrRam ram(
  .clk(ram_clk),
  .rst(!rst_n),
  .wr(!initialized),
  .data_in(ram_din),
  .data_out(ram_dout)
);

Initializer initializer(
  .clk(sys_clk),
  .rst_n(rst_n),
  .data(ram_din),
  .data_v(data_v),
  .initialized(initialized)
);

always @(posedge sys_clk or negedge rst_n) begin
  if (!rst_n)
    clk_w <= 1'd0;
  else
    clk_w <= data_v;
end

endmodule
