#include "../include/atomik_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct atomik_device_t {
    int device_id;
    int is_active;
    atomik_genome_t current_genome;
    uint32_t polymorph_seed;
    int otp_mode; // NEW: Burn-after-reading flag
    uint64_t total_in;
    uint64_t total_out;
} atomik_device_internal_t;

static atomik_device_internal_t g_device = {0};

atomik_result_t atomik_init(void) {
    srand((unsigned int)time(NULL));
    printf("[ATOMiK HAL] Hardware Abstraction Layer Initialized.\n");
    return ATOMIK_SUCCESS;
}

atomik_handle_t atomik_open(int device_id) {
    if (device_id != 0) return NULL;
    if (g_device.is_active) return (atomik_handle_t)&g_device;
    memset(&g_device, 0, sizeof(atomik_device_internal_t));
    g_device.is_active = 1;
    printf("[ATOMiK HAL] Device #%d Attached (Simulated 37ns Core).\n", device_id);
    return (atomik_handle_t)&g_device;
}

void atomik_close(atomik_handle_t dev) {
    if (dev) { ((atomik_device_internal_t*)dev)->is_active = 0; printf("[ATOMiK HAL] Device Detached.\n"); }
}

atomik_result_t atomik_load_genome(atomik_handle_t dev, const char* filepath) {
    if (!dev) return ATOMIK_ERR_NO_DEVICE;
    atomik_device_internal_t* d = (atomik_device_internal_t*)dev;
    printf("[ATOMiK HAL] Reading Genome File: '%s' ...\n", filepath);

    FILE* f = fopen(filepath, "rb");
    if (!f) { printf(" [ERR] FILE NOT FOUND.\n"); return ATOMIK_ERR_INVALID_GENOME; }

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || strncmp(magic, "ATOM", 4) != 0) {
        printf(" [ERR] CORRUPT HEADER.\n"); fclose(f); return ATOMIK_ERR_INVALID_GENOME;
    }

    // NEW HEADER PARSING: Ver(1) + Freq(4) + Policy(1)
    uint8_t version;
    uint32_t poly_freq;
    uint8_t policy;
    fread(&version, 1, 1, f);
    fread(&poly_freq, 4, 1, f);
    fread(&policy, 1, 1, f); // NEW

    fseek(f, 0, SEEK_END);
    long dna_size = ftell(f) - 10; // Total - 10 bytes header
    fclose(f);

    d->current_genome.polymorph_freq_ms = poly_freq;
    d->otp_mode = (policy & 1); // Extract bit 0
    snprintf((char*)d->current_genome.genome_id, 16, "G_VER_%d", version);

    printf("             > [SIG] HEADER VALID (ATOM v%d)\n", version);
    printf("             > [CFG] Polymorphism: %d ms | OTP Mode: %s\n", 
           poly_freq, d->otp_mode ? "ON (Burn-After-Reading)" : "OFF");
    
    if (poly_freq > 0) atomik_set_polymorphism(dev, 0xCAFEBABE, poly_freq);
    return ATOMIK_SUCCESS;
}

atomik_result_t atomik_set_polymorphism(atomik_handle_t dev, uint64_t seed, uint32_t frequency_ms) {
    if (!dev) return ATOMIK_ERR_NO_DEVICE;
    atomik_device_internal_t* d = (atomik_device_internal_t*)dev;
    d->polymorph_seed = (uint32_t)seed;
    d->current_genome.polymorph_freq_ms = frequency_ms;
    printf("[ATOMiK HAL] POLYMORPHISM ENABLED. Seed: 0x%X\n", d->polymorph_seed);
    return ATOMIK_SUCCESS;
}

atomik_result_t atomik_secure_send(atomik_handle_t dev, const void* data, size_t len) {
    if (!dev) return ATOMIK_ERR_NO_DEVICE;
    atomik_device_internal_t* d = (atomik_device_internal_t*)dev;

    d->total_in += len;
    d->total_out += (size_t)(len * 0.08); // Compression simulation

    printf("[ATOMiK SECURE IO] Sending %zu bytes -> [", len);
    for(size_t i=0; i < (len > 10 ? 10 : len); i++) printf("%02X", rand() % 0xFF);
    if (len > 10) printf("...");
    printf("]\n");

    // NEW: Burn-After-Reading Logic simulation
    if (d->otp_mode) {
        d->polymorph_seed = rand(); // Burn the key
        printf("[ATOMiK HAL] >> KEY BURNED. Hardware Map Rotated immediately.\n");
    }

    return ATOMIK_SUCCESS;
}

atomik_result_t atomik_get_metrics(atomik_handle_t dev, atomik_metrics_t* out_metrics) {
    if (!dev || !out_metrics) return ATOMIK_ERR_NO_DEVICE;
    atomik_device_internal_t* d = (atomik_device_internal_t*)dev;
    out_metrics->events_processed = d->total_in;
    out_metrics->grid_save_percent = d->total_in > 0 ? (1.0 - ((double)d->total_out/d->total_in))*100.0 : 0.0;
    out_metrics->entropy_score = 100 + (rand()%100);
    out_metrics->current_watts = 0.05;
    return ATOMIK_SUCCESS;
}