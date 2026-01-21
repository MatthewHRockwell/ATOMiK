#ifndef ATOMIK_CORE_H
#define ATOMIK_CORE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ATOMiK HARDWARE CONSTANTS
 * ========================================================================= */
#define ATOMIK_VERSION_MAJOR      1
#define ATOMIK_VERSION_MINOR      0
#define ATOMIK_LATENCY_NS         37      // The Hardware Guarantee
#define ATOMIK_MAX_BUFFER_SIZE    4096    // Standard Page Size

/* =========================================================================
 * ERROR CODES
 * ========================================================================= */
typedef enum {
    ATOMIK_SUCCESS              = 0,
    ATOMIK_ERR_NO_DEVICE        = -1,     // FPGA not found
    ATOMIK_ERR_INVALID_GENOME   = -2,     // Genome file corrupted
    ATOMIK_ERR_POLYMORPH_FAIL   = -3,     // Scramble sync failed
    ATOMIK_ERR_TIMEOUT          = -4      // Hardware unresponsive
} atomik_result_t;

/* =========================================================================
 * DATA STRUCTURES
 * ========================================================================= */

// Handle to a physical ATOMiK core (or array of cores)
typedef struct atomik_device_t* atomik_handle_t;

// The "DNA" of the application (Blockchain Rules, DoD Keys, etc.)
typedef struct {
    uint8_t   genome_id[16];      // UUID of this configuration
    uint32_t  register_map[256];  // The initial instruction set
    uint8_t   compression_dict[8];// The 8-bit Token Dictionary
    uint32_t  polymorph_freq_ms;  // How often to scramble (0 = static)
} atomik_genome_t;

// Real-time Telemetry (for Dashboard/GreenGrid)
typedef struct {
    uint64_t events_processed;    // Total In
    uint64_t events_emitted;      // Total Out
    double   current_watts;       // Power consumption (estimated)
    double   grid_save_percent;   // Efficiency metric (e.g., 92.0)
    uint32_t entropy_score;       // Polymorphic complexity
} atomik_metrics_t;

/* =========================================================================
 * CORE API (The Engine)
 * ========================================================================= */

/**
 * Initializes the ATOMiK Runtime and scans for FPGA hardware.
 * Must be called before any other function.
 */
atomik_result_t atomik_init(void);

/**
 * Opens a handle to a specific ATOMiK Core.
 * @param device_id Index of the device (0 for single board)
 * @return Handle to device, or NULL on failure.
 */
atomik_handle_t atomik_open(int device_id);

/**
 * Closes the handle and powers down the core (if supported).
 */
void atomik_close(atomik_handle_t dev);

/* =========================================================================
 * VERTICAL 1: GENOME MANAGEMENT (Blockchain/App Layer)
 * ========================================================================= */

/**
 * Loads a "Genome" file into the FPGA.
 * This instantly reconfigures the hardware logic (Instruction Set).
 * * Usage: atomik_load_genome(dev, "ethereum_v2.gnm");
 */
atomik_result_t atomik_load_genome(atomik_handle_t dev, const char* filepath);

/**
 * Validates if the current hardware state matches the expected Genome.
 * Critical for "Zero Trust" verification.
 */
atomik_result_t atomik_verify_genome(atomik_handle_t dev, const atomik_genome_t* expected);

/* =========================================================================
 * VERTICAL 2: DEFENSE LAYER (Polymorphism)
 * ========================================================================= */

/**
 * Sets the "Moving Target" parameters.
 * @param seed The entropy seed for the random number generator.
 * @param frequency_ms How often (in milliseconds) the register map rotates.
 */
atomik_result_t atomik_set_polymorphism(atomik_handle_t dev, uint64_t seed, uint32_t frequency_ms);

/**
 * Creates a Secure Tunnel. Data passed here is routed through
 * randomized registers, making it opaque to side-channel attacks.
 */
atomik_result_t atomik_secure_send(atomik_handle_t dev, const void* data, size_t len);

/* =========================================================================
 * VERTICAL 3: GREEN GRID (Energy/Compute)
 * ========================================================================= */

/**
 * Sets the Sparsity Filter threshold.
 * @param threshold_percent Data below this relevance is dropped (0-100).
 * Default is 90%.
 */
atomik_result_t atomik_set_sparsity(atomik_handle_t dev, float threshold_percent);

/**
 * Retrieves the "Green Score" and other telemetry from the hardware counters.
 * This is a zero-latency register read.
 */
atomik_result_t atomik_get_metrics(atomik_handle_t dev, atomik_metrics_t* out_metrics);

#ifdef __cplusplus
}
#endif

#endif // ATOMIK_CORE_H