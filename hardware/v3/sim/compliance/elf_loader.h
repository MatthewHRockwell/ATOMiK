// =============================================================================
// ATOMiK v3 Minimal ELF64 Loader
//
// Parses ELF64 binaries from riscv-tests, loads PT_LOAD segments into
// memory model, and extracts the 'tohost' symbol address.
// =============================================================================

#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// ELF64 data types
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

// ELF header
struct Elf64_Ehdr {
    unsigned char e_ident[16];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
};

// Program header
struct Elf64_Phdr {
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

// Section header
struct Elf64_Shdr {
    Elf64_Word  sh_name;
    Elf64_Word  sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr  sh_addr;
    Elf64_Off   sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word  sh_link;
    Elf64_Word  sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
};

// Symbol table entry
struct Elf64_Sym {
    Elf64_Word  st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half  st_shndx;
    Elf64_Addr  st_value;
    Elf64_Xword st_size;
};

#define PT_LOAD     1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3

class ElfLoader {
public:
    uint64_t entry_point;
    uint64_t tohost_addr;
    bool     has_tohost;

    ElfLoader() : entry_point(0), tohost_addr(0), has_tohost(false) {}

    // Load ELF file, populate memory via write callback
    // write_cb(addr, data, len) — writes 'len' bytes from 'data' to 'addr'
    template<typename WriteCb>
    bool load(const char *filename, WriteCb write_cb) {
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            fprintf(stderr, "ERROR: Cannot open ELF file: %s\n", filename);
            return false;
        }

        // Read entire file
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        uint8_t *buf = new uint8_t[fsize];
        if ((long)fread(buf, 1, fsize, fp) != fsize) {
            fprintf(stderr, "ERROR: Failed to read ELF file\n");
            fclose(fp);
            delete[] buf;
            return false;
        }
        fclose(fp);

        // Parse ELF header
        if (fsize < (long)sizeof(Elf64_Ehdr)) {
            fprintf(stderr, "ERROR: File too small for ELF header\n");
            delete[] buf;
            return false;
        }

        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buf;

        // Verify ELF magic
        if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
            ehdr->e_ident[2] != 'L'  || ehdr->e_ident[3] != 'F') {
            fprintf(stderr, "ERROR: Not an ELF file\n");
            delete[] buf;
            return false;
        }

        // Verify 64-bit
        if (ehdr->e_ident[4] != 2) {
            fprintf(stderr, "ERROR: Not a 64-bit ELF\n");
            delete[] buf;
            return false;
        }

        // Verify RISC-V (machine = 243)
        if (ehdr->e_machine != 243) {
            fprintf(stderr, "ERROR: Not a RISC-V ELF (machine=%d)\n", ehdr->e_machine);
            delete[] buf;
            return false;
        }

        entry_point = ehdr->e_entry;

        // Load PT_LOAD segments
        for (int i = 0; i < ehdr->e_phnum; i++) {
            Elf64_Phdr *phdr = (Elf64_Phdr *)(buf + ehdr->e_phoff + i * ehdr->e_phentsize);
            if (phdr->p_type == PT_LOAD) {
                uint64_t addr = phdr->p_paddr;  // Use physical address
                uint64_t filesz = phdr->p_filesz;
                uint64_t memsz = phdr->p_memsz;

                // Copy file data
                if (filesz > 0 && phdr->p_offset + filesz <= (uint64_t)fsize) {
                    write_cb((uint32_t)addr, buf + phdr->p_offset, (uint32_t)filesz);
                }

                // Zero-fill BSS (memsz > filesz)
                if (memsz > filesz) {
                    uint8_t *zeros = new uint8_t[memsz - filesz];
                    memset(zeros, 0, memsz - filesz);
                    write_cb((uint32_t)(addr + filesz), zeros, (uint32_t)(memsz - filesz));
                    delete[] zeros;
                }
            }
        }

        // Find 'tohost' symbol in symbol table
        has_tohost = false;
        for (int si = 0; si < ehdr->e_shnum; si++) {
            Elf64_Shdr *shdr = (Elf64_Shdr *)(buf + ehdr->e_shoff + si * ehdr->e_shentsize);
            if (shdr->sh_type == SHT_SYMTAB) {
                // Get associated string table
                Elf64_Shdr *strtab = (Elf64_Shdr *)(buf + ehdr->e_shoff +
                                      shdr->sh_link * ehdr->e_shentsize);
                char *strings = (char *)(buf + strtab->sh_offset);

                int nsyms = shdr->sh_size / shdr->sh_entsize;
                for (int j = 0; j < nsyms; j++) {
                    Elf64_Sym *sym = (Elf64_Sym *)(buf + shdr->sh_offset + j * shdr->sh_entsize);
                    const char *name = strings + sym->st_name;
                    if (strcmp(name, "tohost") == 0) {
                        tohost_addr = sym->st_value;
                        has_tohost = true;
                    }
                }
            }
        }

        if (!has_tohost) {
            fprintf(stderr, "WARNING: 'tohost' symbol not found\n");
        }

        delete[] buf;
        return true;
    }
};

#endif // ELF_LOADER_H
