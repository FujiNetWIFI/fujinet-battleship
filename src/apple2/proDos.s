        .export     _prodosQuit
MLI :=  $bf00
QUIT := $65
.proc      _prodosQuit
        inc $3f4
        jsr MLI
        .byte   QUIT
        .addr   param
param:
        .byte   4
        .byte   0
        .word   0
        .byte   0
        .word   0
.endproc