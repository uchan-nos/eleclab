module Test();

reg clk, rst;
wire [8:0] data;
wire data_v, initialized;

Initializer init(
  .clk(clk),
  .rst_n(~rst),
  .data(data),
  .data_v(data_v),
  .initialized(initialized)
);

always #5 begin
  clk <= ~clk;
end

always @(posedge clk) begin
  if (data_v)
    $display("%d: data = %x", $time, data);
end

initial begin
  $monitor("%d: data_v=%b, data=%x, angle=%d, cor_angle=%d",
           $time, data_v, data, init.angle_deg_lin, init.cor_angle_deg);
  $dumpfile("test.vcd");
  $dumpvars(0, rst, clk, data_v, data, initialized, init.angle_deg_lin, init.cor_angle_deg);
end

initial begin
  clk <= 1;
  rst <= 1;
  #1
  rst <= 0;
  #1000
  $finish;
end

endmodule
