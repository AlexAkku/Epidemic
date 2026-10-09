; MSVC x64 /Gh and /GH enter before the normal calling convention prologue.
; Preserve GP/flags and enabled SIMD xstate, with real Windows unwind metadata.
EXTERN auditor_exact_enter:PROC
EXTERN auditor_exact_exit:PROC
EXTERN auditor_exact_disabled:DWORD
EXTERN __chkstk:PROC
.code
AUDITOR_HOOK MACRO symbol, callback
LOCAL legacy_save, saved, legacy_restore, restored
PUBLIC symbol
symbol PROC FRAME
    push rbp
    .pushreg rbp
    push rbx
    .pushreg rbx
    pushfq
    .allocstack 8
    push rax
    .allocstack 8
    push rcx
    .allocstack 8
    push rdx
    .allocstack 8
    push r8
    .allocstack 8
    push r9
    .allocstack 8
    push r10
    .allocstack 8
    push r11
    .allocstack 8
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog
    ; No collector configured: preserve GP/flags but skip costly CPUID/XSAVE.
    ; Active captures still execute the complete ABI-preserving path below.
    cmp dword ptr [auditor_exact_disabled], 1
    je restored
    ; RBP describes the fixed frame even while RSP is dynamically aligned.
    and rsp, -64
    mov rax, 4160
    call __chkstk
    sub rsp, rax
    ; Shadow space 0..31, mask 32, save kind 40, 64-aligned xstate at 64.
    mov eax, 1
    cpuid
    and ecx, 0C000000h
    cmp ecx, 0C000000h
    jne legacy_save
    xor ecx, ecx
    xgetbv
    and eax, 0E7h
    mov [rsp+32], eax
    mov dword ptr [rsp+40], 1
    ; XRSTOR requires reserved xstate-header bytes to be zero.
    xor edx, edx
    mov [rsp+576], rdx
    mov [rsp+584], rdx
    mov [rsp+592], rdx
    mov [rsp+600], rdx
    mov [rsp+608], rdx
    mov [rsp+616], rdx
    mov [rsp+624], rdx
    mov [rsp+632], rdx
    xsave64 [rsp+64]
    jmp saved
legacy_save:
    mov dword ptr [rsp+40], 0
    fxsave64 [rsp+64]
saved:
    mov rcx, [rbp+80]
    sub rcx, 5
    call callback
    cmp dword ptr [rsp+40], 0
    je legacy_restore
    mov eax, [rsp+32]
    xor edx, edx
    xrstor64 [rsp+64]
    jmp restored
legacy_restore:
    fxrstor64 [rsp+64]
restored:
    push qword ptr [rbp+56]
    popfq
    mov r11, [rbp]
    mov r10, [rbp+8]
    mov r9, [rbp+16]
    mov r8, [rbp+24]
    mov rdx, [rbp+32]
    mov rcx, [rbp+40]
    mov rax, [rbp+48]
    ; Canonical epilog: frame LEA, nonvolatile pops, RET (no POPFQ inside).
    lea rsp, [rbp+64]
    pop rbx
    pop rbp
    ret
symbol ENDP
ENDM
AUDITOR_HOOK _penter, auditor_exact_enter
AUDITOR_HOOK _pexit, auditor_exact_exit
END
