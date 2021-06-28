bits 64
section .data
lotsa:  times 9999 db "a"
.len:   equ $-lotsa
section .text
global  _start
_start: mov rax,1       ; sys_write
        mov rdi,1       ; stdout
        mov rsi,lotsa
        mov rdx,lotsa.len
        syscall
        mov rax,60      ; sys_exit
        mov rdi,0       ; exit code
        syscall
