.data           
    msg: .ascii "Hello World\n"

.text            

.globl _start

_start:
    movq $1, %rax
    movq $1, %rdi
    movq $msg, %rsi
    movq $12, %rdx

    syscall
    movq $60, %rax
    movq $0, %rdi
    syscall
