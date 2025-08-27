module main(
    input rst_n,
    input sys_clk,
    output [5:0] led
);

logic [31:0] counter;

always @(negedge rst_n, posedge sys_clk) begin
    if (~rst_n)
        counter <= 0;
    else if (counter == 32'd27_000_000 - 1)
        counter <= 0;
    else
        counter <= counter + 1;
end

assign led = counter > 32'd10_000_000 ? 6'b000110 : 6'b111001;

endmodule