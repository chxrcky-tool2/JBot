
    .text
    .align 2

    .global asmprof_ticks
    .hidden asmprof_ticks
    .type asmprof_ticks, %function
asmprof_ticks:
    isb
    mrs x0, cntvct_el0
    ret
    .size asmprof_ticks, .-asmprof_ticks

    .global asmprof_freq
    .hidden asmprof_freq
    .type asmprof_freq, %function
asmprof_freq:
    mrs x0, cntfrq_el0
    ret
    .size asmprof_freq, .-asmprof_freq

    .global asmprof_ema
    .hidden asmprof_ema
    .type asmprof_ema, %function
asmprof_ema:
    sub x2, x1, x0
    asr x2, x2, #4
    add x0, x0, x2
    ret
    .size asmprof_ema, .-asmprof_ema
