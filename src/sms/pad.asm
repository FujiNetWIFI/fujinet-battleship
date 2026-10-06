; pad.asm -- note at every frame interrupt which pad buttons are down.
;
; The main loop spends much of its time inside HTTP round trips, and a tap of
; a button that starts and ends inside one would never be polled. From the
; 5 Card Stud SMS client, widened to both buttons. Installed with
; add_raster_int() by src/sms/input.c; crt0's handler saves AF, BC, DE and HL
; around it. input.c clears _padSeen each time it polls.

    SECTION code_user

    PUBLIC  _padIrq
    PUBLIC  _padSeen

_padIrq:
    in      a,($DC)
    cpl                             ; active low -> set bit = held
    and     $30                     ; buttons 1 and 2
    ret     z
    ld      hl,_padSeen
    or      (hl)
    ld      (hl),a
    ret

    SECTION bss_user

_padSeen:
    defb    0
