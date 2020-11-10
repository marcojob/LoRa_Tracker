OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

LD_FEATURE ("SANE_EXPR")

MEMORY {
    FLASH : ORIGIN = 0x08000000, LENGTH = 64K /* flash starts at 0x08000000 and is 64K long */
}

SECTIONS {
    .vectors : {
        *(.vectors)
    } > FLASH /* put at beginning of flash */

    .text : {
        *(.text*)
    } > FLASH /* put at beginning of flash */

}


