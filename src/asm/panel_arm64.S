
    .equ TAG_PANEL, 0x5A01
    .equ TAG_TITLE, 10
    .equ TAG_AVG,   11
    .equ TAG_PEAK,  12
    .equ TAG_MIN,   13

    .equ PANEL_Z,   100

    .equ S_FREQ, 0
    .equ S_EMA,  8
    .equ S_PEAK, 16
    .equ S_MIN,  24
    .equ S_PTIM, 32
    .equ S_TTIM, 36

    .section .rodata
.Lfont:   .asciz "chatFont.fnt"
.Ltitle:  .asciz "JBOT"
.Lempty:  .asciz ""
.Lp_avg:  .asciz "avg  "
.Lp_peak: .asciz "peak "
.Lp_min:  .asciz "min  "

    .section .bss
    .balign 16
.Lstate:
    .space 48

    .text
    .balign 4

.Lto_us:
    movz x2, #0x4240
    movk x2, #0xF, lsl #16
    mul  x0, x0, x2
    lsr  x3, x1, #1
    add  x0, x0, x3
    udiv x0, x0, x1
    ret

.Lfmt:
1:  ldrb w3, [x1], #1
    cbz  w3, 2f
    strb w3, [x0], #1
    b    1b
2:  sub  sp, sp, #32
    add  x5, sp, #24
    mov  x7, x5
    mov  x6, #10
3:  udiv x8, x2, x6
    msub x9, x8, x6, x2
    add  w9, w9, #'0'
    strb w9, [x7, #-1]!
    mov  x2, x8
    cbnz x2, 3b
4:  ldrb w9, [x7], #1
    strb w9, [x0], #1
    cmp  x7, x5
    b.lo 4b
    mov  w9, #' '
    strb w9, [x0], #1
    mov  w9, #'u'
    strb w9, [x0], #1
    mov  w9, #'s'
    strb w9, [x0], #1
    strb wzr, [x0]
    add  sp, sp, #32
    ret

.Lset_line:
    stp  x29, x30, [sp, #-96]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    mov  x19, x0
    mov  w20, w1
    add  x0, sp, #32
    mov  x1, x2
    mov  x2, x3
    bl   .Lfmt
    mov  x0, x19
    mov  w1, w20
    bl   _ZN7cocos2d6CCNode13getChildByTagEi
    cbz  x0, 1f
    add  x1, sp, #32
    bl   _ZN7cocos2d13CCLabelBMFont9setStringEPKc
1:  ldp  x19, x20, [sp, #16]
    ldp  x29, x30, [sp], #96
    ret

.Lbuild_panel:
    stp  x29, x30, [sp, #-80]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    stp  x21, x22, [sp, #32]
    mov  x19, x0

    movz w0, #0x9600, lsl #16
    str  w0, [sp, #48]
    add  x0, sp, #48
    movz w1, #0x4366, lsl #16
    fmov s0, w1
    movz w1, #0x42E8, lsl #16
    fmov s1, w1
    bl   _ZN7cocos2d12CCLayerColor6createERKNS_10_ccColor4BEff
    mov  x20, x0
    cbz  x20, 9f

    mov  x0, x19
    mov  x1, x20
    mov  w2, #PANEL_Z
    mov  w3, #TAG_PANEL
    bl   _ZN7cocos2d6CCNode8addChildEPS0_ii

    mov  w22, #0
1:  adrp x0, .Lempty
    add  x0, x0, :lo12:.Lempty
    adrp x1, .Ltitle
    add  x1, x1, :lo12:.Ltitle
    cmp  w22, #0
    csel x0, x1, x0, eq
    adrp x1, .Lfont
    add  x1, x1, :lo12:.Lfont
    bl   _ZN7cocos2d13CCLabelBMFont6createEPKcS2_
    cbz  x0, 2f
    mov  x21, x0

    str  wzr, [sp, #48]
    fmov s0, #1.0
    str  s0, [sp, #52]
    mov  x0, x21
    add  x1, sp, #48
    bl   _ZN7cocos2d6CCNode14setAnchorPointERKNS_7CCPointE

    scvtf s0, w22
    movz w0, #0x41D0, lsl #16
    fmov s1, w0
    fmul s0, s0, s1
    movz w0, #0x42D8, lsl #16
    fmov s2, w0
    fsub s0, s2, s0
    fmov s1, #10.0
    stp  s1, s0, [sp, #48]
    mov  x0, x21
    add  x1, sp, #48
    bl   _ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE

    mov  x0, x20
    mov  x1, x21
    mov  w2, #1
    add  w3, w22, #TAG_TITLE
    bl   _ZN7cocos2d6CCNode8addChildEPS0_ii
2:  add  w22, w22, #1
    cmp  w22, #4
    b.lo 1b
9:  mov  x0, x20
    ldp  x19, x20, [sp, #16]
    ldp  x21, x22, [sp, #32]
    ldp  x29, x30, [sp], #80
    ret

    .global asmprof_panel_frame
    .hidden asmprof_panel_frame
    .type asmprof_panel_frame, %function
asmprof_panel_frame:
    stp  x29, x30, [sp, #-176]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    stp  x21, x22, [sp, #32]
    stp  x23, x24, [sp, #48]
    str  s0, [sp, #64]
    mov  x19, x0
    mov  x22, x1
    mov  w21, w2
    adrp x20, .Lstate
    add  x20, x20, :lo12:.Lstate

    ldr  x3, [x20, #S_FREQ]
    cbnz x3, 1f
    mrs  x3, cntfrq_el0
    cbnz x3, 0f
    movz x3, #0xF800
    movk x3, #0x124, lsl #16
0:  str  x3, [x20, #S_FREQ]
1:

    ldr  x0, [x20, #S_EMA]
    cbz  x0, 2f
    mov  x1, x22
    bl   asmprof_ema
    b    3f
2:  mov  x0, x22
3:  str  x0, [x20, #S_EMA]

    ldr  x0, [x20, #S_PEAK]
    cmp  x22, x0
    csel x0, x22, x0, hi
    str  x0, [x20, #S_PEAK]

    ldr  x0, [x20, #S_MIN]
    cmp  x22, x0
    csel x1, x22, x0, lo
    cmp  x0, #0
    csel x0, x22, x1, eq
    str  x0, [x20, #S_MIN]

    ldr  w0, [x20, #S_PTIM]
    add  w0, w0, #1
    cmp  w0, #240
    b.lo 4f
    mov  w0, #0
    str  x22, [x20, #S_PEAK]
    str  x22, [x20, #S_MIN]
4:  str  w0, [x20, #S_PTIM]

    ldr  w0, [x20, #S_TTIM]
    add  w0, w0, #1
    cmp  w0, #20
    b.lo 5f
    str  wzr, [x20, #S_TTIM]
    b    6f
5:  str  w0, [x20, #S_TTIM]
    b    .Lexit
6:  cbz  x19, .Lexit

    mov  x0, x19
    mov  w1, #TAG_PANEL
    bl   _ZN7cocos2d6CCNode13getChildByTagEi
    cbnz x0, 7f
    mov  x0, x19
    bl   .Lbuild_panel
    cbz  x0, .Lexit
7:  mov  x22, x0

    and  w1, w21, #1
    mov  x0, x22
    bl   _ZN7cocos2d6CCNode10setVisibleEb
    cbz  w21, .Lexit

    mov  x0, x22
    ldr  s0, [sp, #64]
    bl   _ZN7cocos2d6CCNode8setScaleEf

    bl   _ZN7cocos2d10CCDirector14sharedDirectorEv
    add  x8, sp, #128
    bl   _ZN7cocos2d10CCDirector10getWinSizeEv
    ldp  s0, s1, [sp, #128]
    ldr  s2, [sp, #64]
    movz w0, #0x42E8, lsl #16
    fmov s3, w0
    fmul s3, s3, s2
    fsub s1, s1, s3
    fmov s4, #6.0
    fsub s1, s1, s4
    stp  s4, s1, [sp, #144]
    mov  x0, x22
    add  x1, sp, #144
    bl   _ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE

    ldr  x23, [x20, #S_FREQ]

    ldr  x0, [x20, #S_EMA]
    mov  x1, x23
    bl   .Lto_us
    mov  x3, x0
    mov  x0, x22
    mov  w1, #TAG_AVG
    adrp x2, .Lp_avg
    add  x2, x2, :lo12:.Lp_avg
    bl   .Lset_line

    ldr  x0, [x20, #S_PEAK]
    mov  x1, x23
    bl   .Lto_us
    mov  x3, x0
    mov  x0, x22
    mov  w1, #TAG_PEAK
    adrp x2, .Lp_peak
    add  x2, x2, :lo12:.Lp_peak
    bl   .Lset_line

    ldr  x0, [x20, #S_MIN]
    mov  x1, x23
    bl   .Lto_us
    mov  x3, x0
    mov  x0, x22
    mov  w1, #TAG_MIN
    adrp x2, .Lp_min
    add  x2, x2, :lo12:.Lp_min
    bl   .Lset_line

.Lexit:
    ldp  x19, x20, [sp, #16]
    ldp  x21, x22, [sp, #32]
    ldp  x23, x24, [sp, #48]
    ldp  x29, x30, [sp], #176
    ret
    .size asmprof_panel_frame, .-asmprof_panel_frame

    .section .note.GNU-stack,"",%progbits
