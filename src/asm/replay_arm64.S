    .equ R_DATA,   0
    .equ R_COUNT,  8
    .equ R_CAP,    12
    .equ R_CURSOR, 16
    .equ R_MODE,   20
    .equ R_TPS,    24

    .equ MODE_RECORD, 1
    .equ MODE_PLAY,   2

    .macro REP reg
    adrp \reg, .Lrep
    add  \reg, \reg, :lo12:.Lrep
    .endm

    .macro FUNC name
    .global \name
    .hidden \name
    .type \name, %function
\name:
    .endm

    .macro ENDF name
    .size \name, .-\name
    .endm

    .section .bss
    .balign 16
.Lrep:
    .space 48

    .text
    .balign 4

.Llb:
    ldr  x2, [x0, #R_DATA]
    ldr  w3, [x0, #R_COUNT]
    mov  w4, #0
1:  cmp  w4, w3
    b.hs 3f
    add  w5, w4, w3
    lsr  w5, w5, #1
    add  x7, x2, x5, lsl #3
    ldr  w6, [x7]
    cmp  w6, w1
    b.lo 2f
    mov  w3, w5
    b    1b
2:  add  w4, w5, #1
    b    1b
3:  mov  w0, w4
    ret

    FUNC jbot_rep_mode_set
    REP  x1
    str  w0, [x1, #R_MODE]
    ret
    ENDF jbot_rep_mode_set

    FUNC jbot_rep_mode
    REP  x1
    ldr  w0, [x1, #R_MODE]
    ret
    ENDF jbot_rep_mode

    FUNC jbot_rep_clear
    REP  x0
    str  wzr, [x0, #R_COUNT]
    str  wzr, [x0, #R_CURSOR]
    ret
    ENDF jbot_rep_clear

    FUNC jbot_rep_count
    REP  x1
    ldr  w0, [x1, #R_COUNT]
    ret
    ENDF jbot_rep_count

    FUNC jbot_rep_cursor
    REP  x1
    ldr  w0, [x1, #R_CURSOR]
    ret
    ENDF jbot_rep_cursor

    FUNC jbot_rep_data
    REP  x1
    ldr  x0, [x1, #R_DATA]
    ret
    ENDF jbot_rep_data

    FUNC jbot_rep_length
    REP  x1
    ldr  w2, [x1, #R_COUNT]
    cbz  w2, 1f
    ldr  x3, [x1, #R_DATA]
    sub  w2, w2, #1
    add  x3, x3, x2, lsl #3
    ldr  w0, [x3]
    ret
1:  mov  w0, #0
    ret
    ENDF jbot_rep_length

    FUNC jbot_rep_tps_set
    REP  x1
    str  s0, [x1, #R_TPS]
    ret
    ENDF jbot_rep_tps_set

    FUNC jbot_rep_tps_get
    REP  x1
    ldr  s0, [x1, #R_TPS]
    fcmp s0, #0.0
    b.ne 1f
    movz w2, #0x4370, lsl #16
    fmov s0, w2
1:  ret
    ENDF jbot_rep_tps_get

    FUNC jbot_rep_seek
    stp  x29, x30, [sp, #-16]!
    mov  x29, sp
    REP  x9
    mov  w1, w0
    mov  x0, x9
    bl   .Llb
    str  w0, [x9, #R_CURSOR]
    ldp  x29, x30, [sp], #16
    ret
    ENDF jbot_rep_seek

    FUNC jbot_rep_truncate
    stp  x29, x30, [sp, #-16]!
    mov  x29, sp
    REP  x9
    mov  w1, w0
    mov  x0, x9
    bl   .Llb
    str  w0, [x9, #R_COUNT]
    ldr  w1, [x9, #R_CURSOR]
    cmp  w1, w0
    csel w1, w1, w0, ls
    str  w1, [x9, #R_CURSOR]
    ldp  x29, x30, [sp], #16
    ret
    ENDF jbot_rep_truncate

    FUNC jbot_rep_poll
    REP  x2
    ldr  w3, [x2, #R_MODE]
    cmp  w3, #MODE_PLAY
    b.ne 8f
    ldr  w3, [x2, #R_CURSOR]
    ldr  w4, [x2, #R_COUNT]
    cmp  w3, w4
    b.hs 8f
    ldr  x5, [x2, #R_DATA]
    add  x5, x5, x3, lsl #3
    ldp  w6, w7, [x5]
    cmp  w6, w0
    b.hi 8f
    str  w7, [x1]
    add  w3, w3, #1
    str  w3, [x2, #R_CURSOR]
    mov  w0, #1
    ret
8:  mov  w0, #0
    ret
    ENDF jbot_rep_poll

    FUNC jbot_rep_record
    stp  x29, x30, [sp, #-64]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    stp  x21, x22, [sp, #32]
    REP  x19
    ldr  w4, [x19, #R_MODE]
    cmp  w4, #MODE_RECORD
    b.ne 8f
    and  w1, w1, #0xFF
    and  w2, w2, #1
    and  w3, w3, #1
    orr  w2, w2, w3, lsl #1
    orr  w20, w2, w1, lsl #8
    mov  w21, w0
    ldr  w4, [x19, #R_COUNT]
    ldr  w5, [x19, #R_CAP]
    cmp  w4, w5
    b.lo 1f
    cbnz w5, 2f
    mov  w5, #1024
    b    3f
2:  mov  w6, #0x10000000
    cmp  w5, w6
    b.hs 8f
    lsl  w5, w5, #1
3:  mov  w22, w5
    ldr  x0, [x19, #R_DATA]
    lsl  x1, x22, #3
    bl   realloc
    cbz  x0, 8f
    str  x0, [x19, #R_DATA]
    str  w22, [x19, #R_CAP]
1:  ldr  x0, [x19, #R_DATA]
    ldr  w4, [x19, #R_COUNT]
    add  x0, x0, x4, lsl #3
    stp  w21, w20, [x0]
    add  w4, w4, #1
    str  w4, [x19, #R_COUNT]
    mov  w0, #1
    b    9f
8:  mov  w0, #0
9:  ldp  x19, x20, [sp, #16]
    ldp  x21, x22, [sp, #32]
    ldp  x29, x30, [sp], #64
    ret
    ENDF jbot_rep_record

    FUNC jbot_rep_load
    stp  x29, x30, [sp, #-48]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    str  x21, [sp, #32]
    REP  x19
    mov  x20, x0
    mov  w21, w1
    mov  w6, #0x10000000
    cmp  w21, w6
    b.hi 8f
    cbz  w21, 5f
    ldr  x0, [x19, #R_DATA]
    lsl  x1, x21, #3
    bl   realloc
    cbz  x0, 8f
    str  x0, [x19, #R_DATA]
    str  w21, [x19, #R_CAP]
    mov  x1, x20
    lsl  x2, x21, #3
    bl   memcpy
5:  str  w21, [x19, #R_COUNT]
    str  wzr, [x19, #R_CURSOR]
    mov  w0, #1
    b    9f
8:  mov  w0, #0
9:  ldp  x19, x20, [sp, #16]
    ldr  x21, [sp, #32]
    ldp  x29, x30, [sp], #48
    ret
    ENDF jbot_rep_load

    FUNC jbot_rep_free
    stp  x29, x30, [sp, #-32]!
    mov  x29, sp
    str  x19, [sp, #16]
    REP  x19
    ldr  x0, [x19, #R_DATA]
    bl   free
    str  xzr, [x19, #R_DATA]
    str  wzr, [x19, #R_COUNT]
    str  wzr, [x19, #R_CAP]
    str  wzr, [x19, #R_CURSOR]
    ldr  x19, [sp, #16]
    ldp  x29, x30, [sp], #32
    ret
    ENDF jbot_rep_free

    .section .note.GNU-stack,"",%progbits
