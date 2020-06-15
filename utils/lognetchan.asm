BITS 32

EXTERN printf
GLOBAL lognetchan

SECTION .data
	fmt db "netchan: %i: %i -> %i", 10, 0

SECTION .text

; function c
lognetchan:
	sub esp, 12
	mov eax, [esp+0x34]   ;fragmentstart
L1					; put on hold for debugging
	cmp eax, 15600
	jz L1

	mov [esp+8], eax
	mov [esp+12], ebp     ;fragmentLength
	mov eax, [esp+0x30]   ;fragmented
	mov [esp+4], eax
	mov eax, fmt
	mov [esp], eax
	call printf
	add esp, 12
	mov eax, 0x807e9c2
	jmp eax

