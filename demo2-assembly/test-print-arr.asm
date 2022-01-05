.data
    arr: .int 3, 2, 1, 4, 5, 6
    arr_len: .int 6

.extern print_arr_int

.text
.global _start

_start:
    pushq %rbp
    movq %rsp, %rbp

    # 参数需要压栈
    subq $16, %rsp
    movl %edi, -4(%rbp)
    movq %rsi, -16(%rbp)

    # 参数传递顺序: RID  RSI  RDX  RCX  R8  R9  XMM0-7
    movl arr_len(%rip), %eax
    movl %eax, %esi
    leaq arr(%rip), %rax
    movq %rax, %rdi

    call print_arr_int

    movq $60, %rax
    syscall
    
