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
 *  Modified: 3-stage pipeline with pre-registered RNG for 74+ MHz.
 *  Stage 0: Cursor advance, RNG advance, pre-register control flags
 *  Stage 1: Color from pre-registered RNG + bar overrides + bolt check
 *  Stage 2: Output mux
 */

`timescale 1ns / 1ps
`include "svo_defines.vh"

module svo_tcard #( `SVO_DEFAULT_PARAMS ) (
	input clk, resetn,

	// output stream
	//   tuser[0] ... start of frame
	output reg out_axis_tvalid,
	input out_axis_tready,
	output reg [SVO_BITS_PER_PIXEL-1:0] out_axis_tdata,
	output reg [0:0] out_axis_tuser
);
	`SVO_DECLS

	localparam HOFFSET = ((32 - (SVO_HOR_PIXELS % 32)) % 32) / 2;
	localparam VOFFSET = ((32 - (SVO_VER_PIXELS % 32)) % 32) / 2;

	localparam HOR_CELLS = (SVO_HOR_PIXELS + 31) / 32;
	localparam VER_CELLS = (SVO_VER_PIXELS + 31) / 32;

	localparam BAR_W = (HOR_CELLS - 8 - HOR_CELLS%2) / 2;

	localparam X1 =  2;
	localparam X2 = 2 + BAR_W;
	localparam X3 = HOR_CELLS - 4 - BAR_W;
	localparam X4 = HOR_CELLS - 4;

	function integer best_y_params;
		input integer n, which;
		integer best_y_blk;
		integer best_y_off;
		integer best_y_gap;
		begin
			best_y_blk = 0;
			best_y_gap = 0;
			best_y_off = 0;

			if (SVO_VER_PIXELS == 480) begin
				best_y_blk = 3;
				best_y_gap = 1;
				best_y_off = 1;
			end

			if (SVO_VER_PIXELS == 600) begin
				best_y_blk = 3;
				best_y_gap = 2;
				best_y_off = 2;
			end

			if (SVO_VER_PIXELS == 720) begin
				best_y_blk = 4;
				best_y_gap = 2;
				best_y_off = 1;
			end

			if (SVO_VER_PIXELS == 768) begin
				best_y_blk = 4;
				best_y_gap = 3;
				best_y_off = 2;
			end

			if (SVO_VER_PIXELS == 1080) begin
				best_y_blk = 6;
				best_y_gap = 2;
				best_y_off = 5;
			end

			if (which == 1) best_y_params = best_y_blk;
			if (which == 2) best_y_params = best_y_gap;
			if (which == 3) best_y_params = best_y_off;
		end
	endfunction

	localparam Y_BLK = best_y_params(VER_CELLS, 1);
	localparam Y_GAP = best_y_params(VER_CELLS, 2);
	localparam Y_OFF = best_y_params(VER_CELLS, 3);

	localparam Y1 = 0*Y_BLK + 0*Y_GAP + Y_OFF;
	localparam Y2 = 1*Y_BLK + 0*Y_GAP + Y_OFF;
	localparam Y3 = 1*Y_BLK + 1*Y_GAP + Y_OFF;
	localparam Y4 = 2*Y_BLK + 1*Y_GAP + Y_OFF;
	localparam Y5 = 2*Y_BLK + 2*Y_GAP + Y_OFF;
	localparam Y6 = 3*Y_BLK + 2*Y_GAP + Y_OFF;

	wire [32*32-1:0] bolt_bitmap = {
		32'b 00000000000000000000000000000000,
		32'b 01111111000000000000000001111111,
		32'b 01111100000000000000000000011111,
		32'b 01110000000000000000000000000111,
		32'b 01100000000000000000000000000011,
		32'b 01100000000000000000000000000011,
		32'b 01000000000000000000000000000001,
		32'b 01000000000000000000000000000001,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000111100000000000000,
		32'b 00000000000001111110000000000000,
		32'b 00000000000011111111000000000000,
		32'b 00000000000011111111000000000000,
		32'b 00000000000011111111000000000000,
		32'b 00000000000011111111000000000000,
		32'b 00000000000001111110000000000000,
		32'b 00000000000000111100000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 00000000000000000000000000000000,
		32'b 01000000000000000000000000000001,
		32'b 01000000000000000000000000000001,
		32'b 01100000000000000000000000000011,
		32'b 01100000000000000000000000000011,
		32'b 01110000000000000000000000000111,
		32'b 01111100000000000000000000011111,
		32'b 01111111000000000000000001111111
	};

	// Pipeline enable: all stages advance together
	wire pipe_en = !out_axis_tvalid || out_axis_tready;

	// =========================================================================
	// Cursor state
	// =========================================================================
	reg [`SVO_XYBITS-1:0] hcursor, vcursor;
	reg [`SVO_XYBITS-6:0] x, y;
	reg [4:0] xoff, yoff;

	// =========================================================================
	// Pre-registered RNG and control flags
	// These break the hcursor → color/y critical path by 1 cycle
	// =========================================================================
	reg [31:0] rng;
	reg update_color_r;  // registered: should next stage update color?
	reg grid_edge_r;     // registered: are we on a grid edge?
	reg hcursor_is_last; // registered: hcursor == SVO_HOR_PIXELS-1
	reg hcursor_is_zero; // registered: hcursor == 0
	reg vcursor_is_last; // registered: vcursor == SVO_VER_PIXELS-1

	// =========================================================================
	// Stage 0: Cursor advance + RNG advance + pre-register flags
	// Critical path: just cursor increment (~3 levels with pre-registered comparisons)
	// =========================================================================
	always @(posedge clk) begin
		if (!resetn) begin
			hcursor <= 0;
			vcursor <= 0;
			x <= 0;
			y <= 0;
			xoff <= HOFFSET;
			yoff <= VOFFSET;
			rng <= 0;
			update_color_r <= 1;
			grid_edge_r <= 0;
			hcursor_is_last <= 0;
			hcursor_is_zero <= 1;
			vcursor_is_last <= 0;
		end else
		if (pipe_en) begin
			// Advance RNG from REGISTERED value (no hcursor dependency in chain)
			if (hcursor_is_zero)
				rng <= y ^ 32'd123456789;
			else begin
				rng <= (rng ^ (rng << 13)) ^ (((rng ^ (rng << 13)) ^ ((rng ^ (rng << 13)) >> 17)) << 5)
				     ^ ((rng ^ (rng << 13)) >> 17);
			end

			// Pre-register control flags for NEXT stage 1 cycle
			update_color_r <= (!xoff || hcursor_is_zero);
			grid_edge_r <= (&xoff || &yoff);

			// Advance cursors using pre-registered comparison flags
			if (hcursor_is_last) begin
				hcursor <= 0;
				hcursor_is_zero <= 1;
				hcursor_is_last <= 0;
				x <= 0;
				xoff <= HOFFSET;
				if (vcursor_is_last) begin
					vcursor <= 0;
					vcursor_is_last <= 0;
					y <= 0;
					yoff <= VOFFSET;
				end else begin
					vcursor <= vcursor + 1;
					vcursor_is_last <= (vcursor == SVO_VER_PIXELS-2);
					if (&yoff)
						y <= y + 1;
					yoff <= yoff + 1;
				end
			end else begin
				hcursor <= hcursor + 1;
				hcursor_is_zero <= 0;
				hcursor_is_last <= (hcursor == SVO_HOR_PIXELS-2);
				if (&xoff)
					x <= x + 1;
				xoff <= xoff + 1;
			end
		end
	end

	// =========================================================================
	// Stage 1: Color from pre-registered RNG + color bar overrides + bolt
	// Critical path: rng[bits] → r/g/b → bar override mux (~5-6 levels)
	// No hcursor dependency! rng, update_color_r, grid_edge_r are all registered.
	// =========================================================================
	reg [SVO_BITS_PER_RED-1:0] s1_r;
	reg [SVO_BITS_PER_GREEN-1:0] s1_g;
	reg [SVO_BITS_PER_BLUE-1:0] s1_b;
	reg s1_is_bolt;
	reg s1_bolt_pixel;
	reg s1_sof;
	reg s1_valid;

	// Intermediate combinational color (blocking assignments)
	reg [SVO_BITS_PER_RED-1:0] r;
	reg [SVO_BITS_PER_GREEN-1:0] g;
	reg [SVO_BITS_PER_BLUE-1:0] b;

	always @(posedge clk) begin
		if (!resetn) begin
			s1_valid <= 0;
			s1_r <= 0;
			s1_g <= 0;
			s1_b <= 0;
			s1_is_bolt <= 0;
			s1_bolt_pixel <= 0;
			s1_sof <= 0;
		end else
		if (pipe_en) begin
			// Compute color from REGISTERED rng bits
			if (update_color_r) begin
				r = 16 * rng[0] + 16 * rng[1] + 31 * rng[2];
				g = 16 * rng[3] + 16 * rng[4] + 31 * rng[5];
				b = 16 * rng[6] + 16 * rng[7] + 31 * rng[8];

				if ({r, g, b} == 0) begin
					r = 32;
					g = 32;
					b = 32;
				end
			end

			if (grid_edge_r) begin
				r = 0;
				g = 0;
				b = 0;
			end

			// Start with base color
			s1_r <= r;
			s1_g <= g;
			s1_b <= b;

			// Color bar overrides (using REGISTERED x, y)
			if (SVO_VER_PIXELS >= 480) begin
				if (X1 < x && x <= X2 && Y1 < y && y <= Y2) begin
					s1_r <= 63; s1_g <= 0; s1_b <= 0;
				end

				if (X1 < x && x <= X2 && Y3 < y && y <= Y4) begin
					s1_r <= 0; s1_g <= 63; s1_b <= 0;
				end

				if (X1 < x && x <= X2 && Y5 < y && y <= Y6) begin
					s1_r <= 0; s1_g <= 0; s1_b <= 63;
				end

				if (X3 < x && x <= X4 && Y1 < y && y <= Y2) begin
					s1_r <= 0; s1_g <= 63; s1_b <= 63;
				end

				if (X3 < x && x <= X4 && Y3 < y && y <= Y4) begin
					s1_r <= 63; s1_g <= 0; s1_b <= 63;
				end

				if (X3 < x && x <= X4 && Y5 < y && y <= Y6) begin
					s1_r <= 63; s1_g <= 63; s1_b <= 0;
				end

				if (&xoff && (x == X2 || x == X4)) begin
					s1_r <= 0; s1_g <= 0; s1_b <= 0;
				end

				if (&yoff && (y == Y2 || y == Y4 || y == Y6)) begin
					s1_r <= 0; s1_g <= 0; s1_b <= 0;
				end
			end

			// Bolt region detection
			s1_is_bolt <= (x == 1 || x == HOR_CELLS-2) &&
			              (y == 1 || y == VER_CELLS-2);
			s1_bolt_pixel <= bolt_bitmap[{yoff, xoff}];
			s1_sof <= hcursor_is_zero && !vcursor;
			s1_valid <= 1;
		end
	end

	// =========================================================================
	// Stage 2: Output mux (bolt bitmap vs color) and valid/tuser
	// Critical path: s1_is_bolt → 2-input mux → out (~2 levels)
	// =========================================================================
	always @(posedge clk) begin
		if (!resetn) begin
			out_axis_tvalid <= 0;
			out_axis_tdata <= 0;
			out_axis_tuser <= 0;
		end else
		if (pipe_en) begin
			out_axis_tvalid <= s1_valid;
			out_axis_tdata <= s1_is_bolt ? (s1_bolt_pixel ? ~0 : 0) : {s1_b, s1_g, s1_r};
			out_axis_tuser[0] <= s1_sof;
		end
	end

endmodule
