.section .data
    arr: .int 3, 2, 1, 4, 5, 6
    arr_len: .int 6

.extern print_arr_int

.global _start

_start:
    pushq %rbp
    movq %rsp, %rbp

    movl arr_len, %esi
    movl arr, %edi

    call print_arr_int

    leave
    ret
