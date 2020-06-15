; ET's pure checksum routines
; generated with utils/obj2yasm
;
; CopyRight kobject_
;
; exports purechecksum, ep for et's purechecksum
; prototype:
;
; uint32 __cdecl purechecksum
;	(
;		void *msg,				the message (crc32's of pk3 files)
; 		int mlen,				message length
; 		int checksumfeed,		server supplied checksumfeed
;	);
;
; et.x86 2.60
;
;807a450 func_a
;8079fe0 func_b
;8079e80 func_c

BITS 32

EXTERN memcpy
EXTERN memset

GLOBAL purechecksum

SECTION .data
	DW1	dd	0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

SECTION .text

; function c
func_c:
	push   ebp
	xor    ecx,ecx
	push   edi
	push   esi
	xor    esi,esi
	push   ebx
	mov    edi,[esp+0x1c]
	mov    ebp,[esp+0x14]
	mov    ebx,[esp+0x18]
	cmp    ecx,edi
	jae    M2
	nop
	lea    esi,[esi+0x0]

M1
	movzx  eax,byte [ecx+ebx+0x1]
	movzx  edx,byte [ecx+ebx]
	shl    eax,0x8
	or     edx,eax
	movzx  eax,byte [ecx+ebx+0x2]
	shl    eax,0x10
	or     edx,eax
	movzx  eax,byte [ecx+ebx+0x3]
	add    ecx,0x4
	shl    eax,0x18
	or     edx,eax
	mov    [ebp+esi*4+0x0],edx
	inc    esi
	cmp    ecx,edi
	jb     M1

M2
	pop    ebx
	pop    esi
	pop    edi
	pop    ebp
	ret
;endp

; function a
func_a:
	push   ebp
	mov    ecx,0x40
	push   edi
	push   esi
	push   ebx
	sub    esp,0x6c
	mov    edx,[esp+0x80]
	lea    eax,[esp+0x20]
	mov    esi,[edx+0x4]
	mov    edi,[edx+0x8]
	mov    ebp,[edx+0xc]
	mov    ebx,[edx]
	mov    edx,[esp+0x84]
	mov    [esp+0x8],ecx
	mov    [esp],eax
	mov    [esp+0x4],edx
	call   func_c
	mov    ecx,esi
	mov    eax,[esp+0x20]
	mov    edx,esi
	not    ecx
	and    edx,edi
	and    ecx,ebp
	or     edx,ecx
	add    edx,eax
	add    ebx,edx
	rol    ebx,0x3
	mov    eax,[esp+0x3c]
	mov    edx,ebx
	mov    ecx,ebx
	not    edx
	and    edx,edi
	and    ecx,esi
	or     ecx,edx
	mov    edx,[esp+0x24]
	add    ecx,edx
	add    ebp,ecx
	rol    ebp,0x7
	mov    edx,ebp
	mov    ecx,ebp
	not    edx
	and    edx,esi
	and    ecx,ebx
	or     ecx,edx
	mov    edx,[esp+0x28]
	add    ecx,edx
	add    edi,ecx
	rol    edi,0xb
	mov    edx,edi
	mov    ecx,edi
	not    edx
	and    ecx,ebp
	and    edx,ebx
	or     ecx,edx
	mov    edx,[esp+0x2c]
	add    ecx,edx
	add    esi,ecx
	rol    esi,0x13
	mov    edx,esi
	mov    ecx,esi
	not    edx
	and    edx,ebp
	and    ecx,edi
	or     ecx,edx
	mov    edx,[esp+0x30]
	add    ecx,edx
	add    ebx,ecx
	rol    ebx,0x3
	mov    edx,ebx
	mov    ecx,ebx
	not    edx
	and    ecx,esi
	and    edx,edi
	or     ecx,edx
	mov    edx,[esp+0x34]
	add    ecx,edx
	add    ebp,ecx
	rol    ebp,0x7
	mov    edx,ebp
	mov    ecx,ebp
	not    edx
	and    ecx,ebx
	and    edx,esi
	or     ecx,edx
	mov    edx,[esp+0x38]
	add    ecx,edx
	add    edi,ecx
	rol    edi,0xb
	mov    ecx,edi
	mov    edx,edi
	not    ecx
	and    ecx,ebx
	and    edx,ebp
	or     edx,ecx
	add    edx,eax
	add    esi,edx
	rol    esi,0x13
	mov    edx,esi
	mov    ecx,esi
	not    edx
	and    edx,ebp
	and    ecx,edi
	or     ecx,edx
	mov    edx,[esp+0x40]
	add    ecx,edx
	add    ebx,ecx
	rol    ebx,0x3
	mov    edx,ebx
	mov    ecx,ebx
	not    edx
	and    ecx,esi
	and    edx,edi
	or     ecx,edx
	mov    edx,[esp+0x44]
	add    ecx,edx
	add    ebp,ecx
	rol    ebp,0x7
	mov    edx,ebp
	mov    ecx,ebp
	not    edx
	and    ecx,ebx
	and    edx,esi
	or     ecx,edx
	mov    edx,[esp+0x48]
	add    ecx,edx
	add    edi,ecx
	rol    edi,0xb
	mov    edx,edi
	mov    ecx,edi
	not    edx
	and    edx,ebx
	and    ecx,ebp
	or     ecx,edx
	mov    edx,[esp+0x4c]
	add    ecx,edx
	add    esi,ecx
	rol    esi,0x13
	mov    ecx,esi
	mov    edx,esi
	not    ecx
	and    ecx,ebp
	and    edx,edi
	or     edx,ecx
	mov    ecx,[esp+0x50]
	add    edx,ecx
	add    ebx,edx
	rol    ebx,0x3
	mov    eax,ebx
	mov    edx,ebx
	not    eax
	and    edx,esi
	and    eax,edi
	or     edx,eax
	mov    eax,[esp+0x54]
	add    edx,eax
	add    ebp,edx
	rol    ebp,0x7
	mov    eax,ebp
	mov    edx,ebp
	not    eax
	and    eax,esi
	and    edx,ebx
	or     edx,eax
	mov    eax,[esp+0x58]
	add    edx,eax
	add    edi,edx
	rol    edi,0xb
	mov    edx,edi
	mov    eax,edi
	and    edx,ebp
	mov    [esp+0x1c],edx
	not    eax
	mov    edx,[esp+0x1c]
	and    eax,ebx
	or     eax,edx
	mov    edx,[esp+0x5c]
	add    eax,edx
	add    esi,eax
	mov    edx,[esp+0x1c]
	mov    eax,edi
	rol    esi,0x13
	or     eax,ebp
	and    eax,esi
	or     eax,edx
	mov    edx,[esp+0x20]
	add    eax,edx
	lea    ebx,[eax+ebx+0x5a827999]
	mov    edx,esi
	mov    eax,esi
	rol    ebx,0x3
	or     eax,edi
	and    edx,edi
	and    eax,ebx
	or     eax,edx
	mov    edx,[esp+0x30]
	add    eax,edx
	lea    ebp,[eax+ebp+0x5a827999]
	mov    eax,ebx
	rol    ebp,0x5
	or     eax,esi
	mov    edx,ebx
	and    eax,ebp
	and    edx,esi
	or     eax,edx
	mov    edx,[esp+0x40]
	add    eax,edx
	lea    edi,[eax+edi+0x5a827999]
	mov    edx,ebp
	mov    eax,ebp
	rol    edi,0x9
	or     eax,ebx
	and    edx,ebx
	and    eax,edi
	or     eax,edx
	add    eax,ecx
	lea    esi,[eax+esi+0x5a827999]
	mov    edx,edi
	mov    eax,edi
	rol    esi,0xd
	or     eax,ebp
	and    edx,ebp
	and    eax,esi
	or     eax,edx
	mov    edx,[esp+0x24]
	add    eax,edx
	lea    ebx,[eax+ebx+0x5a827999]
	mov    edx,esi
	mov    eax,esi
	rol    ebx,0x3
	or     eax,edi
	and    edx,edi
	and    eax,ebx
	or     eax,edx
	mov    edx,[esp+0x34]
	add    eax,edx
	lea    ebp,[eax+ebp+0x5a827999]
	mov    eax,ebx
	rol    ebp,0x5
	mov    edx,ebx
	or     eax,esi
	and    edx,esi
	and    eax,ebp
	or     eax,edx
	mov    edx,[esp+0x44]
	add    eax,edx
	lea    edi,[eax+edi+0x5a827999]
	mov    edx,ebp
	mov    eax,ebp
	rol    edi,0x9
	or     eax,ebx
	and    edx,ebx
	and    eax,edi
	or     eax,edx
	mov    edx,[esp+0x54]
	add    eax,edx
	lea    esi,[eax+esi+0x5a827999]
	mov    edx,edi
	mov    eax,edi
	rol    esi,0xd
	or     eax,ebp
	and    edx,ebp
	and    eax,esi
	or     eax,edx
	mov    edx,[esp+0x28]
	add    eax,edx
	lea    ebx,[eax+ebx+0x5a827999]
	mov    eax,esi
	rol    ebx,0x3
	or     eax,edi
	mov    edx,esi
	and    eax,ebx
	and    edx,edi
	or     eax,edx
	mov    edx,[esp+0x38]
	add    eax,edx
	lea    ebp,[eax+ebp+0x5a827999]
	mov    edx,ebx
	mov    eax,ebx
	rol    ebp,0x5
	or     eax,esi
	and    edx,esi
	and    eax,ebp
	or     eax,edx
	mov    edx,[esp+0x48]
	add    eax,edx
	lea    edi,[eax+edi+0x5a827999]
	mov    edx,ebp
	mov    eax,ebp
	rol    edi,0x9
	or     eax,ebx
	and    eax,edi
	and    edx,ebx
	or     eax,edx
	mov    edx,[esp+0x58]
	add    eax,edx
	lea    esi,[eax+esi+0x5a827999]
	mov    eax,edi
	rol    esi,0xd
	or     eax,ebp
	mov    edx,edi
	and    eax,esi
	and    edx,ebp
	or     eax,edx
	mov    edx,[esp+0x2c]
	add    eax,edx
	lea    ebx,[eax+ebx+0x5a827999]
	mov    edx,esi
	mov    eax,esi
	rol    ebx,0x3
	or     eax,edi
	and    edx,edi
	and    eax,ebx
	or     eax,edx
	mov    edx,[esp+0x3c]
	add    eax,edx
	lea    ebp,[eax+ebp+0x5a827999]
	mov    edx,ebx
	mov    eax,ebx
	rol    ebp,0x5
	or     eax,esi
	and    edx,esi
	and    eax,ebp
	or     eax,edx
	mov    edx,[esp+0x4c]
	add    eax,edx
	lea    edi,[eax+edi+0x5a827999]
	mov    eax,ebp
	rol    edi,0x9
	or     eax,ebx
	mov    edx,ebp
	and    eax,edi
	and    edx,ebx
	or     eax,edx
	mov    edx,[esp+0x5c]
	add    eax,edx
	lea    esi,[eax+esi+0x5a827999]
	mov    edx,[esp+0x20]
	rol    esi,0xd
	mov    eax,esi
	xor    eax,edi
	xor    eax,ebp
	add    eax,edx
	lea    ebx,[eax+ebx+0x6ed9eba1]
	mov    edx,[esp+0x40]
	rol    ebx,0x3
	mov    eax,ebx
	xor    eax,esi
	xor    eax,edi
	add    eax,edx
	lea    ebp,[eax+ebp+0x6ed9eba1]
	rol    ebp,0x9
	mov    edx,[esp+0x30]
	mov    eax,ebp
	xor    eax,ebx
	xor    eax,esi
	add    eax,edx
	lea    edi,[eax+edi+0x6ed9eba1]
	rol    edi,0xb
	mov    eax,[esp+0x28]
	mov    edx,edi
	xor    edx,ebp
	xor    edx,ebx
	add    edx,ecx
	lea    esi,[edx+esi+0x6ed9eba1]
	rol    esi,0xf
	mov    ecx,esi
	xor    ecx,edi
	xor    ecx,ebp
	add    ecx,eax
	lea    ebx,[ecx+ebx+0x6ed9eba1]
	mov    eax,[esp+0x48]
	rol    ebx,0x3
	mov    edx,ebx
	xor    edx,esi
	xor    edx,edi
	add    edx,eax
	lea    ebp,[edx+ebp+0x6ed9eba1]
	rol    ebp,0x9
	mov    eax,[esp+0x38]
	mov    ecx,ebp
	xor    ecx,ebx
	xor    ecx,esi
	add    ecx,eax
	lea    edi,[ecx+edi+0x6ed9eba1]
	mov    eax,[esp+0x58]
	rol    edi,0xb
	mov    edx,edi
	xor    edx,ebp
	xor    edx,ebx
	add    edx,eax
	lea    esi,[edx+esi+0x6ed9eba1]
	mov    eax,[esp+0x24]
	rol    esi,0xf
	mov    ecx,esi
	xor    ecx,edi
	xor    ecx,ebp
	add    ecx,eax
	lea    ebx,[ecx+ebx+0x6ed9eba1]
	rol    ebx,0x3
	mov    eax,[esp+0x44]
	mov    edx,ebx
	xor    edx,esi
	xor    edx,edi
	add    edx,eax
	lea    ebp,[edx+ebp+0x6ed9eba1]
	mov    eax,[esp+0x34]
	rol    ebp,0x9
	mov    ecx,ebp
	xor    ecx,ebx
	xor    ecx,esi
	add    ecx,eax
	lea    edi,[ecx+edi+0x6ed9eba1]
	mov    eax,[esp+0x54]
	rol    edi,0xb
	mov    edx,edi
	xor    edx,ebp
	xor    edx,ebx
	add    edx,eax
	lea    esi,[edx+esi+0x6ed9eba1]
	rol    esi,0xf
	mov    eax,[esp+0x2c]
	mov    ecx,esi
	xor    ecx,edi
	xor    ecx,ebp
	add    ecx,eax
	lea    ebx,[ecx+ebx+0x6ed9eba1]
	mov    eax,[esp+0x4c]
	rol    ebx,0x3
	mov    edx,ebx
	xor    edx,esi
	xor    edx,edi
	add    edx,eax
	lea    ebp,[edx+ebp+0x6ed9eba1]
	mov    eax,[esp+0x3c]
	rol    ebp,0x9
	mov    ecx,ebp
	xor    ecx,ebx
	xor    ecx,esi
	add    ecx,eax
	lea    edx,[ecx+edi+0x6ed9eba1]
	rol    edx,0xb
	mov    ecx,[esp+0x5c]
	mov    eax,edx
	xor    eax,ebp
	xor    eax,ebx
	add    eax,ecx
	mov    ecx,[esp+0x80]
	lea    esi,[eax+esi+0x6ed9eba1]
	rol    esi,0xf
	mov    eax,0x40
	add    [ecx],ebx
	add    [ecx+0x4],esi
	add    [ecx+0x8],edx
	xor    edx,edx
	add    [ecx+0xc],ebp
	mov    [esp+0x4],edx
	lea    edx,[esp+0x20]
	mov    [esp+0x8],eax
	mov    [esp],edx
	call   memset
	add    esp,0x6c
	pop    ebx
	pop    esi
	pop    edi
	pop    ebp
	ret
;endp

; function b
func_b:
	push   ebp
	xor    ecx,ecx
	xor    edx,edx
	push   edi
	push   esi
	push   ebx
	sub    esp,0x1c
	mov    esi,[esp+0x34]
	lea    ebx,[esi+0x10]

N1
	movzx  eax,byte [ebx+ecx*4]
	mov    byte [esp+edx+0x10],al
	mov    eax,[ebx+ecx*4]
	inc    ecx
	shr    eax,0x8
	mov    byte [esp+edx+0x11],al
	shr    eax,0x8
	mov    byte [esp+edx+0x12],al
	shr    eax,0x8
	mov    byte [esp+edx+0x13],al
	add    edx,0x4
	cmp    edx,0x8
	jb     N1
	mov    eax,[esi+0x10]
	mov    edi,0x38
	mov    edx,eax
	shr    edx,0x3
	and    edx,0x3f
	cmp    edx,0x37
	jbe    N2
	mov    edi,0x78

N2
	sub    edi,edx
	mov    ecx,eax
	lea    ebx,[edi*8+0x0]
	shr    ecx,0x3
	add    eax,ebx
	and    ecx,0x3f
	mov    [esi+0x10],eax
	cmp    eax,ebx
	jae    N3
	inc    dword [esi+0x14]

N3
	mov    edx,edi
	mov    ebp,0x40
	shr    edx,0x1d
	add    [esi+0x14],edx
	sub    ebp,ecx
	xor    ebx,ebx
	cmp    edi,ebp
	jae    N11

N4
	sub    edi,ebx
	lea    edx,[ebx+DW1]
	lea    ebx,[ecx+esi+0x18]
	mov    [esp+0x8],edi
	mov    ebp,0x8
	mov    [esp+0x4],edx
	mov    [esp],ebx
	call   memcpy
	mov    edi,[esi+0x10]
	mov    edx,edi
	shr    edx,0x3
	add    edi,0x40
	and    edx,0x3f
	cmp    edi,0x40
	mov    [esi+0x10],edi
	jae    N5
	inc    dword [esi+0x14]

N5
	mov    edi,0x40
	xor    ebx,ebx
	sub    edi,edx
	cmp    ebp,edi
	jae    N8

N6
	mov    edi,ebp
	lea    ecx,[esp+0x10]
	add    ecx,ebx
	mov    [esp+0x4],ecx
	lea    ebp,[edx+esi+0x18]
	sub    edi,ebx
	mov    [esp+0x8],edi
	mov    [esp],ebp
	call   memcpy
	xor    ecx,ecx
	xor    edx,edx
	lea    esi,[esi+0x0]
	lea    edi,[edi+0x0]

N7
	movzx  ebx,byte [esi+ecx*4]
	mov    ebp,[esp+0x30]
	mov    byte [edx+ebp],bl
	mov    ebx,[esi+ecx*4]
	shr    ebx,0x8
	mov    byte [edx+ebp+0x1],bl
	movzx  ebx,word [esi+ecx*4+0x2]
	mov    byte [edx+ebp+0x2],bl
	movzx  ebx,byte [esi+ecx*4+0x3]
	inc    ecx
	mov    byte [edx+ebp+0x3],bl
	add    edx,0x4
	cmp    edx,0x10
	jb     N7
	mov    [esp],esi
	mov    eax,0x58
	xor    edx,edx
	mov    [esp+0x8],eax
	mov    [esp+0x4],edx
	call   memset
	add    esp,0x1c
	pop    ebx
	pop    esi
	pop    edi
	pop    ebp
	ret

N8
	mov    [esp+0x8],edi
	lea    ebx,[esp+0x10]
	lea    ecx,[edx+esi+0x18]
	mov    [esp+0x4],ebx
	lea    ebx,[esi+0x18]
	mov    [esp],ecx
	call   memcpy
	mov    [esp+0x4],ebx
	mov    ebx,edi
	mov    [esp],esi
	call   func_a
	lea    ecx,[edi+0x3f]
	cmp    ecx,0x8
	jb     N10

N9
	xor    edx,edx
	jmp    N6

N10
	mov    [esp],esi
	lea    edx,[esp+0x10]
	add    edx,ebx
	mov    [esp+0x4],edx
	add    ebx,0x40
	lea    edi,[ebx+0x3f]
	call   func_a
	cmp    edi,0x8
	jb     N10
	jmp    N9

	N11
	mov    [esp+0x8],ebp
	lea    edx,[ecx+esi+0x18]
	mov    eax,DW1
	mov    [esp+0x4],eax
	lea    ebx,[esi+0x18]
	mov    [esp],edx
	call   memcpy
	mov    [esp+0x4],ebx
	mov    ebx,ebp
	mov    [esp],esi
	call   func_a
	lea    ecx,[ebp+0x3f]
	cmp    ecx,edi
	jb     N13

N12
	xor    ecx,ecx
	jmp    N4
	mov    esi,esi

N13
	mov    [esp],esi
	lea    ecx,[ebx+DW1]
	add    ebx,0x40
	mov    [esp+0x4],ecx
	lea    ebp,[ebx+0x3f]
	call   func_a
	cmp    ebp,edi
	jb     N13
	jmp    N12
;endp

;pure checksum main entry point
purechecksum:
	push   ebp
	mov    edx,0x67452301
	mov    ebp,0xefcdab89
	push   edi
	mov    ecx,0x10325476
	mov    edi,0x98badcfe
	push   esi
	push   ebx
	sub    esp,0x8c
	mov    ebx,0x20
	mov    [esp+0x10],edx
	mov    esi,[esp+0xa4]
	xor    edx,edx
	mov    [esp+0x14],ebp
	mov    [esp+0x18],edi
	mov    [esp+0x1c],ecx
	mov    [esp+0x20],ebx
	mov    [esp+0x24],edx
	mov    eax,0x4
	lea    ebp,[esp+0xa8]
	mov    [esp+0x4],ebp
	lea    edi,[esp+0x28]
	lea    ebx,[esi*8+0x0]
	mov    [esp],edi
	mov    [esp+0x8],eax
	call   memcpy
	mov    ecx,[esp+0x20]
	mov    edx,[esp+0x24]
	mov    ebp,[esp+0x24]
	mov    edi,ecx
	shr    edi,0x3
	add    ecx,ebx
	mov    [esp+0x20],ecx
	inc    edx
	and    edi,0x3f
	cmp    ecx,ebx
	cmovb  ebp,edx
	mov    edx,esi
	xor    ebx,ebx
	mov    [esp+0x24],ebp
	shr    edx,0x1d
	mov    ebp,0x40
	add    [esp+0x24],edx
	sub    ebp,edi
	cmp    esi,ebp
	jae    L2

L1
	sub    esi,ebx
	lea    edx,[esp+edi+0x28]
	lea    ebp,[esp+0x70]
	mov    [esp+0x8],esi
	lea    edi,[esp+0x10]
	mov    esi,[esp+0xa0]
	mov    [esp],edx
	add    esi,ebx
	mov    [esp+0x4],esi
	call   memcpy
	mov    [esp+0x4],edi
	mov    [esp],ebp
	call   func_b
	mov    eax,[esp+0x74]
	mov    ecx,[esp+0x70]
	mov    ebx,[esp+0x78]
	mov    esi,[esp+0x7c]
	xor    eax,ecx
	add    esp,0x8c
	xor    eax,ebx
	xor    eax,esi
	pop    ebx
	pop    esi
	pop    edi
	pop    ebp
	ret
	lea    esi,[esi+0x0]

L2
	mov    [esp+0x8],ebp
	mov    ecx,[esp+0xa0]
	lea    edx,[esp+edi+0x28]
	mov    [esp],edx
	lea    ebx,[esp+0x10]
	lea    edi,[esp+0x28]
	mov    [esp+0x4],ecx
	call   memcpy
	mov    [esp],ebx
	mov    ebx,ebp
	mov    [esp+0x4],edi
	call   func_a
	lea    ecx,[ebp+0x3f]
	cmp    ecx,esi
	jb     L4

L3
	xor    edi,edi
	jmp    L1
	nop

L4
	mov    edx,[esp+0xa0]
	lea    edi,[esp+0x10]
	mov    [esp],edi
	add    edx,ebx
	add    ebx,0x40
	mov    [esp+0x4],edx
	lea    ebp,[ebx+0x3f]
	call   func_a
	cmp    ebp,esi
	jb     L4
	jmp    L3
;endp