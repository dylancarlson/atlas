; Ser.asm Updated 1/27/88 (c) Anticlimactic Teleservices Ltd.

         PAGE    60,132
        TITLE   RS232 utilities for the IBM

Comment         /
******************************************************************
**                                                              **
**              Copyright 1986 Lauren J. Guimont                **
**                                                              **
**      Permission is granted to use this module for any        **
**      personal purposes.  Permission is not granted for       **
**      use of monetary gain or favor without the express       **
**      written permission of the author, Lauren J. Guimont.    **
**                                                              **
**      Regardless of the purpose of use,  removal of the       **
**      copyright message within the DATA segment violates      **
**      all previous rights of use.  In doing so, the user      **
**      gives up all rights to the use of this module in any    **
**      form.                                                   **
**                                                              **
******************************************************************

******************************************************************
**      To convert back to small model, change all 'Far's to    **
**     'Near's.  Change RetAdd 'dd' in StackFrame to 'dw'.      **
*****************************************************************/

RS232_INT       Equ     0014h
RING_BUF_SIZE   Equ     4096d

StackFrame      Struc
  OldBP         dw      ?               ; saved BP value
  RetAdd        dd      ?               ; dw for small memory model
  Arg1          dw      ?               ; first argument passed
  Arg2          dw      ?               ; second argument passed
  Arg3          dw      ?               ; third argument passed
  Arg4          dw      ?               ; fourth argument passed
  Arg5          dw      ?               ; fifth argument passed
  Arg6          dw      ?               ; sixth argument passed
StackFrame      EndS

;
; These are currently set to support the small model of the
; Microsoft C compiler.
;
IGROUP  Group   _Text
DGROUP  Group   _Data

        Assume  cs:IGROUP, ds:DGROUP, es:Nothing, ss:DGROUP

        SUBTTL  Data area
        PAGE

_Data   Segment         Word Public 'DATA'

                db      'Copyright 1986 Lauren J. Guimont'

CommX           dw      0
RS_Base         dw      0
OldRS_Off       dw      0
OldRS_Seg       dw      0
Head            dw      0
Tail            dw      0

InputBuffer     db      RING_BUF_SIZE dup(0)

_Data   EndS

        SUBTTL  Public declarations
        PAGE

_Text   Segment         Byte Public 'CODE'

        Public  _InitRS
        Public  _DeInitRS
        Public  _FlushRS
        Public  _StatRS
        Public  _GetRS
        Public  _PutRS
        Public  _DtrRS
        Public  _CarrStatRS
        Public  _RingStatRS

        Public  _StatCON
        Public  _GetCON
        Public  _PutCON

        SUBTTL  RS232 support routines
        PAGE

;******************************************************************************
;
; Name:                 _InitRS
;
; Synopsis:     void = InitRS(Port, Baud, Stops, Parity, Length);
;
;               int     Port;           0 = COMM1
;                                       1 = COMM2
;               int     Baud;           0 = 110
;                                       1 = 150
;                                       2 = 300
;                                       3 = 600
;                                       4 = 1200
;                                       5 = 2400
;                                       6 = 4800
;                                       7 = 9600
;               int     Stops;          0 = 1 stop bit
;                                       1 = 2 stop bits
;               int     Parity;         0 = none
;                                       1 = odd
;                                       2 = none
;                                       3 = even
;               int     Length;         0 = 5 bit data length
;                                       1 = 6 bit data length
;                                       2 = 7 bit data length
;                                       3 = 8 bit data length
;
; Description:  Initialize one of the COMM ports based upon the passed
;               parameters.
;
; Cautions:     NOTE!
;               There is no error checking on the passed parameters,
;               only that each is masked for the number of bits that
;               will be used for each one.
;
;******************************************************************************
RsInitRecord    Record  Rate:3, Parity:2, Stops:1, WLen:2

_InitRS         Proc    Far
        push    bp
        mov     bp, sp

        push    es
        mov     ax, 40h                 ; BIOS data area
        mov     es, ax
        mov     bx, [bp].Arg1           ; get the COMM number
        and     bx, 1                   ; mask everything else off
        mov     CommX, bx               ; save it for INT control
        shl     bx, 1                   ; make it a word value
        mov     bx, es:[bx]             ; get the base port number
        mov     RS_Base, bx             ; save it
        pop     es

        mov     dx, [bp].Arg1           ; get the COMM number
        sub     bl, bl                  ; clear it for building RsInitRecord

        mov     ax, [bp].Arg2           ; get the baud rate
        mov     cl, Rate                ; get the shift factor
        shl     ax, cl                  ; shift it into place
        and     ax, mask Rate           ; mask the rate
        or      bl, al                  ; OR it into RsInitRecord

        mov     ax, [bp].Arg3           ; get the stop bits
        mov     cl, Stops               ; get the shift factor
        shl     ax, cl                  ; shift it into place
        and     ax, mask Stops          ; mask the stop bits
        or      bl, al                  ; OR it into RsInitRecord

        mov     ax, [bp].Arg4           ; get the parity
        mov     cl, Parity              ; get the shift factor
        shl     ax, cl                  ; shift it into place
        and     ax, mask Parity         ; mask the parity
        or      bl, al                  ; OR it into RsInitRecord

        mov     ax, [bp].Arg5           ; get the word length
        mov     cl, WLen                ; get the shift factor
        shl     ax, cl                  ; shift it into place
        and     ax, mask WLen           ; mask the length
        or      al, bl                  ; get the rest of RsInitRecord
;
; OK......now:
;       AH = 0
;       AL = initialization bit pattern
;       DX = comm number (0 or 1)
;
        int     RS232_INT               ; let BIOS do it
;
; Set up the ring buffer pointers, our interrupt vector,
; set the UART to interrupt on data available, then set
; the interrupt controller to handle the UART requests.
;
        mov     Head, 0                 ; set the head...
        mov     Tail, 0                 ;...and the tail pointers

        push    es
        sub     ax, ax
        mov     es, ax                  ; point to the INT vectors
        mov     bx, 1                   ; preset it
        sub     bx, CommX               ; COMM1 = 1, COMM2 = 0
        add     bx, 0bh                 ; add in the base INT number
        shl     bx, 1                   ; x4 to adjust it to point to the INT
        shl     bx, 1
        mov     ax, es:[bx]             ; get the offset
        mov     OldRS_Off, ax           ; save it
        mov     ax, es:[bx + Word]      ; get the segment
        mov     OldRS_Seg, ax           ; save it

        lea     ax, _Service_RS         ; point to the interrupt routine
        pushf                           ; save the flag status
        cli                             ; no interrupts during this
        mov     es:[bx], ax             ; install the offset
        mov     es:[bx + Word], cs      ; install the vector
        popf                            ; restore int. flag to entry status
        pop     es
;
; Enable UART to generate interrupts on input.
;
        mov     dx, RS_Base             ; get the port address
        inc     dx                      ; InterruptEnableRegister
        mov     al, 1                   ; enable data available mask
        out     dx, al                  ; RS232 data in is now interrupt driven
;
; Set the OUT2 bit in Modem Control Register (enables interrupt line) 
;
        mov     dx, RS_Base             ; get the port address
        in      al, dx                  ; read the serial port to clear it
        add     dx, 4                   ; ModemControlRegister
        in      al, dx                  ; get contents of ModemControlRegister
        or      al, 8                   ; set the OUT2 bit (enables interrupts)
        out     dx, al                  ; Interrupts will be handled properly
;
; Enable the interrupt controller to respond to the UART requests.
;
        mov     bx, 1                   ; preset it
        sub     bx, CommX               ; COMM1 = 1, COMM2 = 0
        inc     bx                      ; set bit 0 or 1
        shl     bx, 1                   ; shift it out to bit 3 or 4
        shl     bx, 1
        shl     bx, 1
        not     bl                      ; invert the mask

        pushf                           ; save the flag status
        cli                             ; no interrupts when setting controller
        in      al, 21h                 ; get current status
        and     al, bl                  ; mask out the COMM bit
        out     21h, al                 ; reset the controller
        popf                            ; restore int. flag to entry status

        pop     bp
        ret
_InitRS         EndP

;******************************************************************************
;
; Name:                 _DeInitRS
;
; Synopsis:     void = DeInitRS();
;
; Description:  Restores the saved INT vector set during _InitRS,
;               resets the UART not to generate interrupts, and
;               resets the interrupt controller not to respond to
;               the UART.
;
;******************************************************************************
_DeInitRs       Proc    Far
        mov     dx, RS_Base             ; get the port address
        inc     dx                      ; InterruptEnableRegister
        sub     al, al                  ; disable all RS232 interrupts
        out     dx, al                  ; its done
;
; Turn off OUT2 bit in Modem Control Register (disables interrupt line) 
;
        mov     dx, RS_Base             ; get the port address
        add     dx, 4                   ; ModemControlRegister
        in      al, dx                  ; get contents of ModemControlRegister
        and     al, 0F7h                ; set the OUT2 bit (disables interrupts)
        out     dx, al                  ; Interrupts will be disabled

        mov     bx, 1                   ; preset it
        sub     bx, CommX               ; COMM1 = 1, COMM2 = 0
        inc     bx                      ; set bit 0 or 1
        shl     bx, 1                   ; shift it out to bit 3 or 4
        shl     bx, 1
        shl     bx, 1

        pushf                           ; save the flag status
        cli                             ; no interrupts when setting controller
        in      al, 21h                 ; get current status
        or      al, bl                  ; force the COMM bit on for disable
        out     21h, al                 ; reset the controller
        popf                            ; restore int. flag to entry status

        push    es
        sub     ax, ax
        mov     es, ax                  ; point to the INT vectors
        mov     bx, 1                   ; preset it
        sub     bx, CommX               ; COMM1 = 1, COMM2 = 0
        add     bx, 0bh                 ; add in the base INT number
        shl     bx, 1                   ; adjust it to point to the INT
        shl     bx, 1
        mov     ax, OldRS_Off           ; get the offset
        mov     cx, OldRS_Seg           ; get the segment
        pushf                           ; save the flag status
        cli                             ; no interrupts during this
        mov     es:[bx], ax             ; reset the offset
        mov     es:[bx + Word], cx      ; reset the segment
        popf                            ; restore int. flag to entry status
        pop     es

        ret
_DeInitRS       EndP

;******************************************************************************
;
; Name:                 _FlushRS
;
; Synopsis:     void FlushRS();
;
; Description:  Reset the RS232 input buffer to empty.
;
;******************************************************************************
_FlushRS        Proc    Far
        sub     ax, ax                  ; clear it to zero
        mov     Head, ax                ; clear the head
        mov     Tail, ax                ; clear the tail
        ret
_FlushRS        EndP

;******************************************************************************
;
; Name:                 _StatRS
;
; Synopsis:     Status = StatRS();
;
;               int     Status;                 current COMM status
;
; Description:  If Status = 0, then the ring buffer is empty, else
;               there is at least one character waiting.
;
;******************************************************************************
_StatRS         Proc    Far
        mov     ax, Head                ; get the head pointer
        sub     ax, Tail                ; if they are the same, its empty
        ret
_StatRS         EndP

;******************************************************************************
;
; Name:                 _GetRS
;
; Synopsis:     Character = GetRS();
;
;               int     Character;              byte from RS232 RX buffer
;
; Description:  Gets the next character from the input ring buffer.
;
; Cautions:     If there is nothing there, it will wait for one.
;
;******************************************************************************
_GetRS  Proc    Far
        mov     ax, Head                ; get the head pointer
        sub     ax, Tail                ; if they are the same, its empty
        jz      _GetRS                  ; wait for something

        mov     bx, Tail                ; get the tail pointer
        mov     al, InputBuffer[bx]     ; get whats there
        sub     ah, ah                  ; clear the MSB
        inc     bx                      ; bump the pointer
        cmp     bx, RING_BUF_SIZE       ; about to wrap?
        jne     SetTheTail              ; nope

        sub     bx, bx                  ; reset for the wrap

SetTheTail:
        mov     Tail, bx                ; update the pointer

Exit_GetRS:
        ret
_GetRS  EndP

;******************************************************************************
;
; Name:                 _PutRS
;
; Synopsis:     void = PutRS(Character);
;
;               char    Character;              byte for RS232 TX buffer
;
; Description:  Stuffs a character into THRE.
;
;******************************************************************************
_PutRS  Proc    Far
        push    bp
        mov     bp, sp

        mov     dx, RS_Base             ; get the base address
        add     dx, 5                   ; line status register

THRE_Status:
        in      al, dx                  ; get the line status
        test    al, 20h                 ; THRE empty?
        jz      THRE_Status             ; not yet

        sub     dx, 5                   ; back to the base register
        mov     ax, [bp].Arg1           ; get the character
        out     dx, al                  ; send it out

        pop     bp
        ret
_PutRS  EndP

;******************************************************************************
;
; Name:                 _DtrRS
;
; Synopsis:     void = DtrRS(Dtr);
;
;               int     Port;           0 = dtr off
;                                       1 = dtr on
;
; Description:  Sets bit 0:DTR of ModemControlRegister according to Dtr.
;
;******************************************************************************
_DtrRS  Proc    Far
        push    bp
        mov     bp, sp
;
; Change the DTR bit in Modem Control Register
;
        mov     dx, RS_Base             ; get the port address
        add     dx, 4                   ; ModemControlRegister
        in      al, dx                  ; get contents of ModemControlRegister

        cmp     [bp].Arg1, 0            ; Dtr on? or Off.
        je      DtrOff                  ; Off, jump to DtrOff

        or      al, 1                   ; DtrOn, turn bit 0:DTR on
        jmp     EndOfDtrRS              ; Jump to set bit

DtrOff:
        and     al, 0FEh                ; DtrOff, turn bit 0:DTR off

EndOfDtrRS:
        out     dx, al 

        pop     bp
        ret
_DtrRS  EndP

;******************************************************************************
;
; Name:                 _CarrStatRS
;
; Synopsis:     Status = CarrStatRS();
;
;               int     Status;                 current COMM carrier status
;
; Description:  If Status = 0, then carrier is not present
;               else there is carrier detected.
;
;******************************************************************************
_CarrStatRS  Proc    Far
        push    bp
        mov     bp, sp

        mov     dx, RS_Base             ; get the port address
        add     dx, 6                   ; ModemStatusRegister
        in      al, dx                  ; get contents of ModemStatusRegister

        and     al, 080h                ; mask out all but bit 7:CD bit
        cmp     al, 080h                ; test cd bit
        mov     ax, 0                   ; assume status is zero

        jne     EndOfCarrStatRS         ; don't set AX 
        mov     ax, 1

EndOfCarrStatRS:
        pop     bp
        ret
_CarrStatRS  EndP

;******************************************************************************
;
; Name:                 _RingStatRS
;
; Synopsis:     Status = RingStatRS();
;
;               int     Status;                 current COMM ring status
;
; Description:  If Status = 0, then ring indicator bit is false
;               else ring indicator bit is true.
;
;******************************************************************************
_RingStatRS  Proc    Far
        push    bp
        mov     bp, sp

        mov     dx, RS_Base             ; get the port address
        add     dx, 6                   ; ModemStatusRegister
        in      al, dx                  ; get contents of ModemStatusRegister

        and     al, 040h                ; mask out all but bit 6:RI bit
        cmp     al, 040h                ; test ri bit
        mov     ax, 0                   ; assume status is zero

        jne     EndOfRingStatRS         ; don't set AX 
        mov     ax, 1

EndOfRingStatRS:
        pop     bp
        ret
_RingStatRS  EndP


        SUBTTL  RS232 interrupt handler
        PAGE

;******************************************************************************
;               R S 2 3 2   I N T E R R U P T   H A N D L E R
;******************************************************************************

_Service_RS     Proc    Far
        sti                             ; interrupts back on
        push    ds
        push    dx
        push    cx
        push    bx
        push    ax

        mov     ax, DGROUP
        mov     ds, ax                  ; get pointer to our data

        mov     dx, RS_Base             ; base address of COMM port
        in      al, dx                  ; get the character
        mov     bx, Head                ; point to the head of the buffer
        mov     cx, bx                  ; get the offset
        inc     cx                      ; bump it
        cmp     cx, RING_BUF_SIZE       ; about to wrap?
        jne     NoWrapYet               ; nope

        sub     cx, cx                  ; reset for the wrap

NoWrapYet:
        cmp     cx, Tail                ; about to overflow?
        je      Short EndOfRS232Service ; yup, get out

        mov     InputBuffer[bx], al     ; save the character
        mov     Head, cx                ; reset the buffer head

EndOfRS232Service:
        mov     al, 20h                 ; EOI command
        out     20h, al                 ; tell the interrupt controller

        pop     ax
        pop     bx
        pop     cx
        pop     dx
        pop     ds
        iret
_Service_RS     EndP

;******************************************************************************
;
; Name:                 _StatCON
;
; Synopsis:     Status = StatCON();
;
;               int     Status;                 current CON status
;
; Description:  If Status = 0, the keyboard buffer is empty, else
;               returns value of the next character and scan code waiting
;               to be fetched by GetCON().
;
;******************************************************************************
_StatCON        Proc    Far
        push    bp
        mov     bp, sp

        mov     ah, 1                   ; read keyboard status
        int     16h                     ; BIOS keyboard service

        jz      NoChar                  ; is there a character?
        jmp     Char                    ; yes, get the character!
NoChar:
        mov     ax, 0                   ; no character, return zero
        pop     bp
        ret
Char:
        cmp     al, 0                   ; extended character?
        je      Extended2                
        xor     ah, ah                  ; no, zero high byte

Extended2:
        pop     bp
        ret
_StatCON        EndP

;******************************************************************************
;
; Name:                 _GetCON
;
; Synopsis:     Character = GetCON();
;
;               int     Character;              int from keyboard buffer
;
; Description:  Gets the next character and scan code from the keyboard buffer.
;               Is not affected by Ctrl-C, Ctrl-S etc.
;
; Cautions:     If there is nothing there, it will wait for one.
;
;******************************************************************************
_GetCON         Proc    Far
        push    bp
        mov     bp, sp

        mov     ah, 0                   ; read keyboard character
        int     16h                     ; BIOS keyboard service

        cmp     al, 0                   ; extended character?
        je      Extended                
        xor     ah, ah                  ; no, zero high byte

Extended:
        pop     bp
        ret
_GetCON         EndP

;******************************************************************************
;
; Name:                 _PutCON
;
; Synopsis:     void = PutCON(Character);
;
;               char    Character;              byte to be written to display
;
; Description:  Writes character but is not affected by Ctrl-C, Ctrl-S etc.
;
;******************************************************************************
_PutCON         Proc    Far
        push    bp
        mov     bp, sp

        mov     ax, [bp].Arg1           ; character to display
        mov     ah, 0eh                 ; write text in teletype mode
        mov     bh, 0                   ; page zero
        int     10h                     ; BIOS video service

        pop     bp
        ret     
_PutCON         EndP

_Text   Ends
        End


