; Use RIP-relative memory addressing
        default rel

; Mark stack as non-executable for Binutils 2.39+
        section .note.GNU-stack noalloc noexec nowrite progbits

    section .text
    global AddSubI32_a
AddSubI32_a:

; Calculate (a + b) - (c + d) + 7
    add edi,esi ;edi = a + b
    add edx,ecx ;edx = c + d
    sub edi,edx ;edi = (a + b) - (c + d)
    add edi, 7  ;edi = (a + b) - (c + d) + 7

    mov eax,edi ;eax = final result

    ret ;return to caller