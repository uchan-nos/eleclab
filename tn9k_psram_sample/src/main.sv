module main(
    input rst_n,
    input ext_clk,
    output [5:0] led,
    output [21:0] dbg1,
    output [7:0] dbg2,
    output [3:0] dbg3,
    output pin68,
    output pin69,
    // PSRAM signals
    output [1:0] O_psram_ck,
    output [1:0] O_psram_ck_n,
    inout [15:0] IO_psram_dq,
    inout [1:0] IO_psram_rwds,
    output [1:0] O_psram_cs_n,
    output [1:0] O_psram_reset_n
);

assign dbg1 = {
    /*    21 */ init_calib,
    /*    20 */ sys_clk,
    /* 19:16 */ state,
    /* 15: 8 */ rd_data[7:0],
    /*  7: 0 */ wr_data[7:0]
};

assign dbg2 = {cmd, cmd_en, addr[5:0]};

assign dbg3 = psram_timing[3:0];

assign pin68 = 0; // GND
assign pin69 = 0; // GND

// クロック分周用
logic [1:0] clk_div;
(* syn_keep = "true" *) logic sys_clk; // syn_keep=true にしておくと、必ずネットリストに現れる

//`define DIVIDE_CLOCK
`ifdef DIVIDE_CLOCK
always @(negedge rst_n, posedge ext_clk) begin
    if (~rst_n) begin
        sys_clk <= 0;
        clk_div <= 0;
    end
    else begin
        if (clk_div == 0) sys_clk <= ~sys_clk;
        clk_div <= clk_div + 2'd1;
    end
end
`else
assign sys_clk = ext_clk;
`endif


logic [31:0] counter;

always @(negedge rst_n, posedge sys_clk) begin
    if (~rst_n)
        counter <= 0;
    else if (counter == 32'd27_000_000 - 1)
        counter <= 0;
    else
        counter <= counter + 1;
end

typedef enum logic [3:0] {
    INITIALIZING,   // 0 初期化完了待ち
    INIT_WAIT,      // 1 初期化後待ち
    WR_COMMAND,     // 2 書き込みコマンド送信中
    WR_BURST,       // 3 書き込み転送中
    WR_COMPLETED,   // 4 書き込み完了、コマンド間隔調整中
    RD_COMMAND,     // 5 読み出しコマンド送信中
    RD_WAIT_VALID,  // 6 読み出し有効化待ち
    RD_BURST,       // 7 読み出し転送中
    RD_COMPLETED    // 8 読み出し完了
} state_t;
state_t state;

logic calib_completed, wr_completed, rd_started, rd_completed, rd_data_is_expected;

always @(negedge rst_n, posedge mem_clk_out) begin
    if (~rst_n) begin
        calib_completed <= 0;
        wr_completed <= 0;
        rd_started <= 0;
        rd_completed <= 0;
        rd_data_is_expected <= 0;
    end
    else if (state == INIT_WAIT)
        calib_completed <= 1;
    else if (state == WR_COMPLETED)
        wr_completed <= 1;
    else if (state == RD_COMMAND)
        rd_started <= 1;
    else if (state == RD_COMPLETED)
        rd_completed <= 1;
    //else if (state == RD_BURST & rd_data == 64'h01234567DEADBEEF)
    else if (rd_data_buf == 16'hBEEF)
        rd_data_is_expected <= 1;
end

assign led = {
    counter < 32'd5_000_000 ? 1'b0 : 1'b1, // led[5]
    ~rd_data_is_expected, // led[4]
    ~rd_completed, // led[3]
    ~rd_started, // led[2]
    ~wr_completed, // led[1]
    ~calib_completed // led[0]
};

logic mem_clk; // 162MHz (PLL output)
logic mem_clk_out; // mem_clk / 2
logic pll_lock;

Gowin_rPLL pll(
    .clkout(mem_clk), //output clkout
    .lock(pll_lock),  //output lock
    .clkin(ext_clk)   //input clkin
);

logic [63:0] wr_data, rd_data;
logic [20:0] addr;
logic rd_data_valid, cmd, cmd_en, init_calib;
logic [7:0] psram_timing;

PSRAM_Memory_Interface_HS_Top psram_hs(
    .clk(sys_clk), //input clk
    .memory_clk(mem_clk), //input memory_clk
    .pll_lock(pll_lock), //input pll_lock
    .rst_n(rst_n), //input rst_n
    .O_psram_ck(O_psram_ck), //output [1:0] O_psram_ck
    .O_psram_ck_n(O_psram_ck_n), //output [1:0] O_psram_ck_n
    .IO_psram_dq(IO_psram_dq), //inout [15:0] IO_psram_dq
    .IO_psram_rwds(IO_psram_rwds), //inout [1:0] IO_psram_rwds
    .O_psram_cs_n(O_psram_cs_n), //output [1:0] O_psram_cs_n
    .O_psram_reset_n(O_psram_reset_n), //output [1:0] O_psram_reset_n
    .wr_data(wr_data), //input [63:0] wr_data
    .rd_data(rd_data), //output [63:0] rd_data
    .rd_data_valid(rd_data_valid), //output rd_data_valid
    .addr(addr), //input [20:0] addr
    .cmd(cmd), //input cmd
    .cmd_en(cmd_en), //input cmd_en
    .init_calib(init_calib), //output init_calib
    .clk_out(mem_clk_out), //output clk_out
    .data_mask(8'd0) //input [7:0] data_mask
);

assign wr_data = make_wr_data(state == WR_COMMAND, psram_timing);
assign addr = 21'h20;
assign cmd = state == WR_COMMAND;
assign cmd_en = state == WR_COMMAND | state == RD_COMMAND;

function [63:0] make_wr_data(logic first, logic [7:0] t);
    casex ({first, t})
        9'h1xx: return 64'h01234567DEADBEEF;
        9'h000: return 64'h76543210BAADF00D;
        9'h001: return 64'hBADDCAFE01234567;
        9'h002: return 64'hFEEDFACE76543210;
        default: return 64'h0;
    endcase
endfunction

/*
    INITIALIZING,   // 初期化完了待ち
    WR_COMMAND,     // 書き込みコマンド送信中
    WR_BURST,       // 書き込み転送中
    WR_COMPLETED,   // 書き込み完了、コマンド間隔調整中
    RD_COMMAND,     // 読み出しコマンド送信中
    RD_WAIT_VALID,  // 読み出し有効化待ち
    RD_BURST,       // 読み出し転送中
    RD_COMPLETED,   // 読み出し完了
*/

always @(negedge rst_n, posedge mem_clk_out) begin
    if (~rst_n)
        state <= INITIALIZING;
    else
        case (state)
            INITIALIZING:   if (init_calib) state <= INIT_WAIT;
            INIT_WAIT:      if (psram_timing == 2) state <= WR_COMMAND;
            WR_COMMAND:     state <= WR_BURST;
            WR_BURST:       if (psram_timing == 2) state <= WR_COMPLETED;
            WR_COMPLETED:   if (psram_timing == 14) state <= RD_COMMAND;
            RD_COMMAND:     state <= RD_WAIT_VALID;
            RD_WAIT_VALID:  if (rd_data_valid) state <= RD_BURST;
            RD_BURST:       if (~rd_data_valid) state <= RD_COMPLETED;
            RD_COMPLETED:   state <= RD_COMPLETED;
        endcase
end

always @(negedge rst_n, posedge mem_clk_out) begin
    if (~rst_n)
        psram_timing <= 0;
    else
        case (state)
            INITIALIZING:   psram_timing <= 0;
            WR_COMMAND:     psram_timing <= 0;
            RD_WAIT_VALID:  psram_timing <= 0;
            RD_COMPLETED:   psram_timing <= 0;
            default:        psram_timing <= psram_timing + 8'd1;
        endcase
end

logic [15:0] rd_data_buf;
always @(negedge rst_n, posedge mem_clk_out) begin
    if (~rst_n)
        rd_data_buf <= 0;
    else
        if (state == RD_WAIT_VALID & rd_data_valid)
            rd_data_buf <= rd_data[15:0];
end

endmodule