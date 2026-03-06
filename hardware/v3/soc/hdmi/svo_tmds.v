/*
 *  SVO - Simple Video Out FPGA Core
 *
 *  Copyright (C) 2014  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  Modified: 3-stage pipeline for high-frequency operation (74+ MHz).
 *  Original was single-cycle combinational (18 LUT levels, max ~38 MHz).
 *  Pipeline adds 2 cycles latency — negligible for HDMI blanking compliance.
 */

`timescale 1ns / 1ps
`include "svo_defines.vh"

module svo_tmds (
	input clk, resetn, de,
	input [1:0] ctrl,
	input [7:0] din,
	output reg [9:0] dout
);

	wire [7:0] D = din;

	function [3:0] N1;
		input [7:0] bits;
		integer i;
		begin
			N1 = 0;
			for (i = 0; i < 8; i = i+1)
				N1 = N1 + bits[i];
		end
	endfunction

	function [3:0] N0;
		input [7:0] bits;
		integer i;
		begin
			N0 = 0;
			for (i = 0; i < 8; i = i+1)
				N0 = N0 + !bits[i];
		end
	endfunction

	// =====================================================================
	// Stage 1: q_m extraction (XOR/XNOR chain from input data)
	// Critical path: N1(D) popcount + comparison + 7-bit XOR/XNOR chain
	// ~7-8 LUT levels
	// =====================================================================
	reg [8:0] q_m_s1;
	reg       de_s1;
	reg [1:0] ctrl_s1;

	always @(posedge clk) begin
		if (!resetn) begin
			q_m_s1  <= 9'b0;
			de_s1   <= 1'b0;
			ctrl_s1 <= 2'b0;
		end else begin
			de_s1   <= de;
			ctrl_s1 <= ctrl;

			if (de) begin
				if ((N1(D) > 4) | ((N1(D) == 4) & (D[0] == 0))) begin
					q_m_s1[0] <=           D[0];
					q_m_s1[1] <= D[0] ^~ D[1];
					q_m_s1[2] <= (D[0] ^~ D[1]) ^~ D[2];
					q_m_s1[3] <= ((D[0] ^~ D[1]) ^~ D[2]) ^~ D[3];
					q_m_s1[4] <= (((D[0] ^~ D[1]) ^~ D[2]) ^~ D[3]) ^~ D[4];
					q_m_s1[5] <= ((((D[0] ^~ D[1]) ^~ D[2]) ^~ D[3]) ^~ D[4]) ^~ D[5];
					q_m_s1[6] <= (((((D[0] ^~ D[1]) ^~ D[2]) ^~ D[3]) ^~ D[4]) ^~ D[5]) ^~ D[6];
					q_m_s1[7] <= ((((((D[0] ^~ D[1]) ^~ D[2]) ^~ D[3]) ^~ D[4]) ^~ D[5]) ^~ D[6]) ^~ D[7];
					q_m_s1[8] <= 1'b0;
				end else begin
					q_m_s1[0] <=          D[0];
					q_m_s1[1] <= D[0] ^ D[1];
					q_m_s1[2] <= (D[0] ^ D[1]) ^ D[2];
					q_m_s1[3] <= ((D[0] ^ D[1]) ^ D[2]) ^ D[3];
					q_m_s1[4] <= (((D[0] ^ D[1]) ^ D[2]) ^ D[3]) ^ D[4];
					q_m_s1[5] <= ((((D[0] ^ D[1]) ^ D[2]) ^ D[3]) ^ D[4]) ^ D[5];
					q_m_s1[6] <= (((((D[0] ^ D[1]) ^ D[2]) ^ D[3]) ^ D[4]) ^ D[5]) ^ D[6];
					q_m_s1[7] <= ((((((D[0] ^ D[1]) ^ D[2]) ^ D[3]) ^ D[4]) ^ D[5]) ^ D[6]) ^ D[7];
					q_m_s1[8] <= 1'b1;
				end
			end
		end
	end

	// =====================================================================
	// Stage 2: Popcount of q_m (N0, N1 computation)
	// Critical path: 8-bit popcount tree (~4 LUT levels)
	// =====================================================================
	reg [3:0] N0_s2, N1_s2;
	reg [8:0] q_m_s2;
	reg       de_s2;
	reg [1:0] ctrl_s2;

	always @(posedge clk) begin
		if (!resetn) begin
			N0_s2   <= 4'b0;
			N1_s2   <= 4'b0;
			q_m_s2  <= 9'b0;
			de_s2   <= 1'b0;
			ctrl_s2 <= 2'b0;
		end else begin
			q_m_s2  <= q_m_s1;
			de_s2   <= de_s1;
			ctrl_s2 <= ctrl_s1;

			if (de_s1) begin
				N0_s2 <= N0(q_m_s1[7:0]);
				N1_s2 <= N1(q_m_s1[7:0]);
			end
		end
	end

	// =====================================================================
	// Stage 3: Disparity arithmetic and output encoding
	// Critical path: cnt comparison + arithmetic + mux (~6-8 LUT levels)
	// =====================================================================
	reg signed [7:0] cnt;
	reg [9:0] q_out;
	reg [9:0] dout_buf2;

	reg [9:0] q_out_next;
	reg signed [7:0] cnt_next, cnt_tmp;

	always @(posedge clk) begin
		if (!resetn) begin
			cnt      <= 0;
			q_out    <= 0;
			dout_buf2 <= 0;
			dout     <= 0;
		end else if (!de_s2) begin
			cnt <= 0;
			case (ctrl_s2)
				2'b00: q_out <= 10'b1101010100;
				2'b01: q_out <= 10'b0010101011;
				2'b10: q_out <= 10'b0101010100;
				2'b11: q_out <= 10'b1010101011;
			endcase
		end else begin
			// Disparity-based encoding (uses registered N0/N1 from stage 2)
			if ((cnt == 0) | (N1_s2 == N0_s2)) begin
				q_out_next[9]    = ~q_m_s2[8];
				q_out_next[8]    =  q_m_s2[8];
				q_out_next[7:0]  = (q_m_s2[8] ? q_m_s2[7:0] : ~q_m_s2[7:0]);
				if (q_m_s2[8] == 0)
					cnt_next = cnt + (N0_s2 - N1_s2);
				else
					cnt_next = cnt + (N1_s2 - N0_s2);
			end else if (((cnt > 0) & (N1_s2 > N0_s2)) | ((cnt < 0) & (N0_s2 > N1_s2))) begin
				q_out_next[9]    =  1'b1;
				q_out_next[8]    =  q_m_s2[8];
				q_out_next[7:0]  = ~q_m_s2[7:0];
				cnt_tmp          = cnt + (N0_s2 - N1_s2);
				if (q_m_s2[8])
					cnt_next = cnt_tmp + 2'h2;
				else
					cnt_next = cnt_tmp;
			end else begin
				q_out_next[9]    =  1'b0;
				q_out_next[8]    =  q_m_s2[8];
				q_out_next[7:0]  =  q_m_s2[7:0];
				cnt_tmp          = cnt + (N1_s2 - N0_s2);
				if (q_m_s2[8])
					cnt_next = cnt_tmp;
				else
					cnt_next = cnt_tmp - 2'h2;
			end
			cnt   <= cnt_next;
			q_out <= q_out_next;
		end

		// Output pipeline (2 additional stages for OSER10 timing)
		dout_buf2 <= q_out;
		dout <= dout_buf2;
	end
endmodule
