// Asm Profiler - cocos2d-x stats panel, written entirely in AArch64 assembly
// (Android arm64-v8a, GNU assembler syntax)
//
// void asmprof_panel_frame(void* parent, uint64_t spent, uint32_t enabled, float scale)
//      x0 = node to attach the panel to (PlayLayer::m_uiLayer), or NULL (stats only)
//      x1 = ticks spent in this update
//      w2 = panel enabled (0/1)
//      s0 = panel scale
//
// What it does:
//   * every call: updates avg (EMA), peak and min of the update time
//   * every 20th call: finds (or builds) the panel under `parent`, then sets
//     visibility, scale, position (top-left corner) and the three text lines
//
// The panel is never cached as a pointer. It is looked up with
// CCNode::getChildByTag() on the live parent each refresh, so there are no
// dangling pointers when a level is left and a new PlayLayer is created.
//
// cocos2d-x functions are called through their Itanium-mangled names; the
// signature each one stands for is written next to the call.

    .equ TAG_PANEL, 0x5A01
    .equ TAG_TITLE, 10
    .equ TAG_AVG,   11
    .equ TAG_PEAK,  12
    .equ TAG_MIN,   13

    .equ PANEL_Z,   100

    // state block layout
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

// ---------------------------------------------------------------------------
// x0 = ticks, x1 = freq  ->  x0 = round(ticks * 1000000 / freq)   (clobbers x2,x3)
// ---------------------------------------------------------------------------
.Lto_us:
    movz x2, #0x4240
    movk x2, #0xF, lsl #16          // 1,000,000
    mul  x0, x0, x2
    lsr  x3, x1, #1
    add  x0, x0, x3
    udiv x0, x0, x1
    ret

// ---------------------------------------------------------------------------
// x0 = out buffer, x1 = prefix C string, x2 = value
// writes "<prefix><decimal value> us\0"        (leaf; clobbers x3-x9)
// ---------------------------------------------------------------------------
.Lfmt:
1:  ldrb w3, [x1], #1
    cbz  w3, 2f
    strb w3, [x0], #1
    b    1b
2:  sub  sp, sp, #32
    add  x5, sp, #24                // end of scratch digits
    mov  x7, x5
    mov  x6, #10
3:  udiv x8, x2, x6
    msub x9, x8, x6, x2             // x9 = x2 % 10
    add  w9, w9, #'0'
    strb w9, [x7, #-1]!
    mov  x2, x8
    cbnz x2, 3b                     // do-while, so 0 prints as "0"
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

// ---------------------------------------------------------------------------
// x0 = panel, w1 = label tag, x2 = prefix, x3 = value (us)
// finds the label by tag and sets its text
// ---------------------------------------------------------------------------
.Lset_line:
    stp  x29, x30, [sp, #-96]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    mov  x19, x0
    mov  w20, w1
    add  x0, sp, #32                // 64 byte text buffer at sp+32
    mov  x1, x2
    mov  x2, x3
    bl   .Lfmt
    mov  x0, x19
    mov  w1, w20
    bl   _ZN7cocos2d6CCNode13getChildByTagEi            // CCNode::getChildByTag(int)
    cbz  x0, 1f
    add  x1, sp, #32
    bl   _ZN7cocos2d13CCLabelBMFont9setStringEPKc       // CCLabelBMFont::setString(const char*)
1:  ldp  x19, x20, [sp, #16]
    ldp  x29, x30, [sp], #96
    ret

// ---------------------------------------------------------------------------
// x0 = parent  ->  x0 = new panel (already added to parent), or 0 on failure
// ---------------------------------------------------------------------------
.Lbuild_panel:
    stp  x29, x30, [sp, #-80]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    stp  x21, x22, [sp, #32]
    mov  x19, x0                    // parent

    // background: CCLayerColor::create(const ccColor4B&, float w, float h)
    movz w0, #0x9600, lsl #16       // ccColor4B {r=0, g=0, b=0, a=0x96}
    str  w0, [sp, #48]
    add  x0, sp, #48
    movz w1, #0x4366, lsl #16       // 230.0f
    fmov s0, w1
    movz w1, #0x42E8, lsl #16       // 116.0f
    fmov s1, w1
    bl   _ZN7cocos2d12CCLayerColor6createERKNS_10_ccColor4BEff
    mov  x20, x0
    cbz  x20, 9f

    mov  x0, x19
    mov  x1, x20
    mov  w2, #PANEL_Z
    mov  w3, #TAG_PANEL
    bl   _ZN7cocos2d6CCNode8addChildEPS0_ii             // CCNode::addChild(CCNode*, int z, int tag)

    // four lines: title + avg/peak/min
    mov  w22, #0
1:  adrp x0, .Lempty
    add  x0, x0, :lo12:.Lempty
    adrp x1, .Ltitle
    add  x1, x1, :lo12:.Ltitle
    cmp  w22, #0
    csel x0, x1, x0, eq             // line 0 starts as the title, others empty
    adrp x1, .Lfont
    add  x1, x1, :lo12:.Lfont
    bl   _ZN7cocos2d13CCLabelBMFont6createEPKcS2_       // CCLabelBMFont::create(const char*, const char*)
    cbz  x0, 2f
    mov  x21, x0

    str  wzr, [sp, #48]             // CCPoint anchor {0, 1}
    fmov s0, #1.0
    str  s0, [sp, #52]
    mov  x0, x21
    add  x1, sp, #48
    bl   _ZN7cocos2d6CCNode14setAnchorPointERKNS_7CCPointE   // CCNode::setAnchorPoint(const CCPoint&)

    scvtf s0, w22                   // y = 108 - 26 * i, x = 10
    movz w0, #0x41D0, lsl #16       // 26.0f
    fmov s1, w0
    fmul s0, s0, s1
    movz w0, #0x42D8, lsl #16       // 108.0f
    fmov s2, w0
    fsub s0, s2, s0
    fmov s1, #10.0
    stp  s1, s0, [sp, #48]
    mov  x0, x21
    add  x1, sp, #48
    bl   _ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE      // CCNode::setPosition(const CCPoint&)

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

// ---------------------------------------------------------------------------
// void asmprof_panel_frame(void* parent, uint64_t spent, uint32_t enabled, float scale)
// ---------------------------------------------------------------------------
    .global asmprof_panel_frame
    .hidden asmprof_panel_frame
    .type asmprof_panel_frame, %function
asmprof_panel_frame:
    stp  x29, x30, [sp, #-176]!
    mov  x29, sp
    stp  x19, x20, [sp, #16]
    stp  x21, x22, [sp, #32]
    stp  x23, x24, [sp, #48]
    str  s0, [sp, #64]              // scale
    mov  x19, x0                    // parent
    mov  x22, x1                    // spent
    mov  w21, w2                    // enabled
    adrp x20, .Lstate
    add  x20, x20, :lo12:.Lstate

    // counter frequency, fetched once (fallback 19.2 MHz)
    ldr  x3, [x20, #S_FREQ]
    cbnz x3, 1f
    mrs  x3, cntfrq_el0
    cbnz x3, 0f
    movz x3, #0xF800
    movk x3, #0x124, lsl #16        // 19,200,000
0:  str  x3, [x20, #S_FREQ]
1:
    // avg: EMA, seeded by the first sample
    ldr  x0, [x20, #S_EMA]
    cbz  x0, 2f
    mov  x1, x22
    bl   asmprof_ema
    b    3f
2:  mov  x0, x22
3:  str  x0, [x20, #S_EMA]

    // peak
    ldr  x0, [x20, #S_PEAK]
    cmp  x22, x0
    csel x0, x22, x0, hi
    str  x0, [x20, #S_PEAK]

    // min (0 = not set yet)
    ldr  x0, [x20, #S_MIN]
    cmp  x22, x0
    csel x1, x22, x0, lo
    cmp  x0, #0
    csel x0, x22, x1, eq
    str  x0, [x20, #S_MIN]

    // peak and min window resets after 240 updates (about 1 s at 240 ups)
    ldr  w0, [x20, #S_PTIM]
    add  w0, w0, #1
    cmp  w0, #240
    b.lo 4f
    mov  w0, #0
    str  x22, [x20, #S_PEAK]
    str  x22, [x20, #S_MIN]
4:  str  w0, [x20, #S_PTIM]

    // text refresh every 20 updates
    ldr  w0, [x20, #S_TTIM]
    add  w0, w0, #1
    cmp  w0, #20
    b.lo 5f
    str  wzr, [x20, #S_TTIM]
    b    6f
5:  str  w0, [x20, #S_TTIM]
    b    .Lexit
6:  cbz  x19, .Lexit

    // find or build the panel under the live parent
    mov  x0, x19
    mov  w1, #TAG_PANEL
    bl   _ZN7cocos2d6CCNode13getChildByTagEi
    cbnz x0, 7f
    mov  x0, x19
    bl   .Lbuild_panel
    cbz  x0, .Lexit
7:  mov  x22, x0                    // panel

    and  w1, w21, #1
    mov  x0, x22
    bl   _ZN7cocos2d6CCNode10setVisibleEb               // CCNode::setVisible(bool)
    cbz  w21, .Lexit

    mov  x0, x22
    ldr  s0, [sp, #64]
    bl   _ZN7cocos2d6CCNode8setScaleEf                  // CCNode::setScale(float)

    // top-left corner: pos = (6, winH - 6 - 116 * scale)
    //
    // CCSize has a user-provided copy constructor, so under AAPCS64 it is NOT
    // returned in s0/s1. getWinSize() returns it through memory: the caller
    // passes the address of the result buffer in x8 (the indirect result
    // register) and `this` in x0. x8 is not preserved across calls, so it must
    // be set right before the call. The old code left x8 as whatever the
    // previous code put there, and getWinSize() copied the CCSize to that
    // garbage address -> SIGSEGV in CCSize::CCSize(CCSize const&).
    bl   _ZN7cocos2d10CCDirector14sharedDirectorEv      // CCDirector::sharedDirector() -> x0
    add  x8, sp, #128                                   // x8 = &CCSize result {w, h} (8 bytes at sp+128)
    bl   _ZN7cocos2d10CCDirector10getWinSizeEv          // CCSize CCDirector::getWinSize()  (sret via x8)
    ldp  s0, s1, [sp, #128]                             // s0 = w, s1 = h
    ldr  s2, [sp, #64]
    movz w0, #0x42E8, lsl #16       // 116.0f
    fmov s3, w0
    fmul s3, s3, s2
    fsub s1, s1, s3
    fmov s4, #6.0
    fsub s1, s1, s4
    stp  s4, s1, [sp, #144]
    mov  x0, x22
    add  x1, sp, #144
    bl   _ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE

    // text lines
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
