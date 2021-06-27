bits 64
section .text
global  _start          ; linker entry point
_start: mov rbp,rsp     ; save pointer to stack pointer
        pop rcx         ; get argument count off stack
_nexta: pop rcx         ; loop popping off arg pointers
        cmp rcx,0       ; ... until 64-bit 0 value
        jne _nexta      ; GOTO!
_nexte: pop rcx         ; loop popping off env pointers
        cmp rcx,0       ; ... until 64-bit 0 value
        jne _nexte      ; GOTO!
        mov rax,60      ; sys_exit syscall
        mov rdi,0       ; exit code
        syscall         ; make it so
