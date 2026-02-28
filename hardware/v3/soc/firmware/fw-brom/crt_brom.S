.section .text

.global crtStart
.global main

crtStart:
  j crtInit
  nop
  nop
  nop
  nop
  nop
  nop
  nop

crtInit:
  # Skip gp initialization - not needed for bringup
  # Set sp using direct load (no AUIPC)
  # Note: ISP main() needs 384-byte stack frame. This value empirically works.
  li sp, 0x800002F0    # Stack start (validated to work with bringup mode)

  # Skip BSS clear - not needed for bringup

  call main
infinitLoop:
  j infinitLoop
