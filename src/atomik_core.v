module atomik_core (
    input wire clk,
    input wire rst_n,          // Controlled by BIOS (core_enable)
    
    // Configuration Inputs (From BIOS)
    input wire [31:0] scramble_threshold, // Frequency (in cycles)
    input wire [31:0] polymorph_seed,     // Initial Seed
    input wire otp_en,                    // NEW: One-Time Pad Mode
    
    // Data Interfaces (Simplified for v3 demo)
    input wire [31:0] data_in,
    input wire data_valid,
    output reg [31:0] data_out,
    output reg data_ready
);

    // --- POLYMORPHIC ENGINE ---
    reg [31:0] current_seed;
    reg [31:0] timer;
    
    // 32-bit Xorshift Logic for Map Rotation
    function [31:0] xorshift32;
        input [31:0] seed;
        begin
            seed = seed ^ (seed << 13);
            seed = seed ^ (seed >> 17);
            seed = seed ^ (seed << 5);
            xorshift32 = seed;
        end
    endfunction

    // --- EXECUTION STATE MACHINE ---
    reg processing_active;
    wire execution_done; 
    assign execution_done = (processing_active && !data_valid); // Trailing edge

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_seed <= polymorph_seed;
            timer <= 0;
            processing_active <= 0;
            data_out <= 0;
            data_ready <= 0;
        end else begin
            // 1. DATA PROCESSING (The "Work")
            if (data_valid) begin
                processing_active <= 1;
                // Simple XOR encryption using current hardware map state
                data_out <= data_in ^ current_seed; 
                data_ready <= 1;
            end else begin
                processing_active <= 0;
                data_ready <= 0;
            end

            // 2. POLYMORPHISM (The "Protection")
            // Rotate if Timer Expires OR if OTP Mode is active and we just finished
            if ( (timer >= scramble_threshold && scramble_threshold > 0) || 
                 (otp_en && execution_done) ) begin
                
                current_seed <= xorshift32(current_seed);
                timer <= 0; // Reset timer
                
            end else if (scramble_threshold > 0) begin
                timer <= timer + 1;
            end
        end
    end
endmodule