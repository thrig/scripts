bits 64
section .data
letter: db "a"
section .text
global  _start
_start: mov r12,9999
        mov rax,1       ; sys_write
        mov rdi,1       ; stdout
        mov rsi,letter
        mov rdx,1       ; length
_again: syscall
        dec r12
        jnz _again      ; GOTO!
        mov rax,60      ; sys_exit
        mov rdi,0       ; exit code
        syscall
