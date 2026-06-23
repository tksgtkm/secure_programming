    .code16
    .global code16, code16_end
guest16:
    movw $42, %ax
    movw %ax, 0x400
guest16_end: