global asm_convert_float_array_to_int

section .text

; int asm_convert_float_array_to_int(const float *input, int *output, int count)
; System V AMD64 ABI:
; rdi = input
; rsi = output
; edx = count
; return eax = 0 on success
asm_convert_float_array_to_int:
    test rdi, rdi
    jz .error

    test rsi, rsi
    jz .error

    test edx, edx
    jl .error
    jz .success

    xor ecx, ecx

.loop:
    movss xmm0, [rdi + rcx * 4]
    cvttss2si eax, xmm0
    mov [rsi + rcx * 4], eax

    inc ecx
    cmp ecx, edx
    jl .loop

.success:
    xor eax, eax
    ret

.error:
    mov eax, -1
    ret
