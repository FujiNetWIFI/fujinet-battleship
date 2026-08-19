' ===========================================================================
' input.bas -- hand controller decoding.
'
' Shared verbatim by every FujiNet Intellivision client (5 Card Stud, Texas
' Hold'Em, Fujitzee, Battleship, Fujirkle). Keep the copies byte-identical.
'
' ---------------------------------------------------------------------------
' Why this file exists
' ---------------------------------------------------------------------------
' The hand controller multiplexes the disc, the three action buttons and the
' 12-key keypad onto the same eight lines at $01FF. A keypad press grounds one
' "row" line (bit 7/6/5) and one "column" line (bit 3/2/1/0) -- and those
' column lines are the *same* lines the disc uses.
'
' IntyBASIC's CONT1.LEFT / .UP / .BUTTON accessors are naive bit masks over
' that raw byte (MVI $01FF / XORI #255 / ANDI #mask), so every keypad press
' aliases straight onto them. Using the constants from constants.bas:
'
'   KEYPAD_CLEAR = $88 -> AND DISC_LEFT ($08) = $08 -> CONT1.LEFT   is true
'                      -> AND BUTTON_MASK($E0) = $80 -> CONT1.BUTTON is true
'   KEYPAD_ENTER = $28 -> also LEFT, also BUTTON
'   KEYPAD_1     = $81 -> DOWN + BUTTON
'   KEYPAD_4     = $82 -> RIGHT + BUTTON
'   KEYPAD_7     = $84 -> UP + BUTTON        ...and so on for all twelve keys.
'
' That is why CLEAR used to fight the play cursor: CONT1.LEFT goes true the
' instant the key is down, while CONT1.KEY only updates inside WAIT once three
' consecutive frames agree, so the spurious cursor move always won the race --
' the move handler consumed the press and re-entered its loop before the
' CONT1.KEY test was ever reached. The IntyBASIC manual notes the hazard under
' CONT1.KEY ("Because movements can be taken as keys, it's suggested to wait
' for CONT1.KEY to contain 12 before waiting for a key").
'
' ---------------------------------------------------------------------------
' How the reading is disambiguated
' ---------------------------------------------------------------------------
' Count the row bits. Each of the three action buttons grounds *two* of them
' (BUTTON_1 = $A0 top, BUTTON_2 = $60 bottom-left, BUTTON_3 = $C0
' bottom-right); a keypad key grounds exactly *one*; the disc alone grounds
' none. So:
'
'   no row bits      -> disc only: publish inp_dir.
'   one row bit      -> a keypad key is down: publish neither disc nor button,
'                       because both are aliases of the key itself.
'   two row bits     -> a real button, and any low bits alongside it are a
'                       genuine simultaneous disc reading, so publish both.
'
' Testing the row-bit count this way (rather than "any low bit and any high
' bit at once") is what keeps a real button-plus-disc hold working -- $A8 is
' the top button with the disc held left, and battleship's ship placement
' relies on that combination staying readable.
'
' Two keypad keys at once still alias to a button pattern (1+9 and 3+7 both
' give $A5, the EXEC's pause combo). That is inherent to the matrix and
' nothing here uses it.
'
' ---------------------------------------------------------------------------
' Usage
' ---------------------------------------------------------------------------
' Call GOSUB read_input exactly once immediately after every WAIT in a loop
' that reads input. It must follow the WAIT, because WAIT is where IntyBASIC
' refreshes its debounced keypad decode. Then use inp_* instead of CONT1.*:
'
'   IF inp_dir AND DISC_LEFT THEN ...    ' level: disc held left
'   IF inp_btn THEN ...                  ' level: any action button held
'   IF inp_btn = BUTTON_2 THEN ...       ' level: bottom-left specifically
'   IF inp_btn_hit THEN ...              ' edge:  a button went down this frame
'   IF inp_key = 11 THEN ...             ' level: ENTER held (hold-to-view)
'   IF inp_key_hit = 10 THEN ...         ' edge:  CLEAR was just pressed
'
' Disc reads stay level-triggered so inp_lock keeps providing auto-repeat.
' The _hit edges fire on exactly one frame per physical press, which is what
' the menu toggles want: a still-held CLEAR can no longer close the menu on
' the same frame it opened it, so none of the old "spin until the key is
' released" loops are needed any more.
'
' Known gap: fujinet.bas's blocking WAIT loops don't call read_input, so a
' press and release that both land inside one network round-trip is missed.
' That is pre-existing behaviour.
' ===========================================================================

CONST CONT1_PORT = $01FF

' Frames the raw port must hold steady before a reading is published. The
' keypad's row and column contacts need not close on the same frame, so the
' first frame of a CLEAR press can read as a bare $08 -- indistinguishable
' from disc-left -- before the row bit arrives. One frame of agreement (16ms)
' rides that out and isn't felt on the disc. Raise it if presses still leak.
CONST INPUT_SETTLE = 1

' All 8-bit on purpose: fujitzee sits at the 47 16-bit variable ceiling, so
' nothing here may take a '#' name.
    DIM inp_raw, inp_dir, inp_btn, inp_key
    DIM inp_btn_hit, inp_key_hit
    DIM inp_btn_prev, inp_key_prev
    DIM inp_new, inp_seen, inp_settle, inp_row, inp_iskey
    DIM inp_lock

read_input: PROCEDURE
    ' One snapshot per frame. Reading the port once (rather than letting each
    ' CONT1.x accessor re-read it) also means every field below is decoded
    ' from the same instant.
    inp_new = PEEK(CONT1_PORT) XOR 255
    IF inp_new <> inp_seen THEN
        inp_seen = inp_new
        inp_settle = INPUT_SETTLE
    ELSE
        IF inp_settle > 0 THEN
            inp_settle = inp_settle - 1
            IF inp_settle = 0 THEN inp_raw = inp_new
        END IF
    END IF

    inp_row = inp_raw AND BUTTON_MASK
    inp_dir = 0
    inp_btn = 0
    ' Exactly one row bit means a keypad key is down. Written as separate IFs
    ' rather than one chained OR -- multi-condition one-liners have bitten
    ' this compiler before.
    inp_iskey = 0
    IF inp_row = $20 THEN inp_iskey = 1
    IF inp_row = $40 THEN inp_iskey = 1
    IF inp_row = $80 THEN inp_iskey = 1
    IF inp_iskey = 0 THEN
        inp_btn = inp_row
        inp_dir = inp_raw AND DISK_MASK
    END IF

    ' IntyBASIC's own debounced decode: 0-9 digits, 10 = CLEAR, 11 = ENTER,
    ' 12 = nothing pressed. Reads a variable, not the port, so it can't
    ' disagree with the snapshot above.
    inp_key = CONT1.KEY

    ' Edge flags. inp_key_hit carries 12 ("no key") when nothing changed, so
    ' both "no edge" and "released back to idle" read as non-actionable.
    inp_key_hit = 12
    IF inp_key <> inp_key_prev THEN
        inp_key_prev = inp_key
        inp_key_hit = inp_key
    END IF
    inp_btn_hit = 0
    IF inp_btn <> inp_btn_prev THEN
        inp_btn_prev = inp_btn
        inp_btn_hit = inp_btn
    END IF
END
