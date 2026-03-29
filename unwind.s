.intel_syntax noprefix

.section .data
asciis:
	.ascii "0123456789abcdef"

.section .text

finish:
	ret

.global unwind;

unwind:	
	#Initiliaze to first 
	push rbp
	mov rbp, rsp
	
read_loop:
	cmp rbp, 1
	jne write_address
	call finish

update_rbp:
	mov r11, '\n' #newline
        push r11
        mov rax, 1                                                                                     
        mov rdi, 1
        mov rsi, rsp
        mov rdx, 1
        syscall
        pop r11

	mov rbp, QWORD PTR [rbp]
	jmp read_loop

write_address: 
	mov r11, '0' #double space
        push r11
	mov rax, 1
        mov rdi, 1
        mov rsi, rsp
        mov rdx, 1
        syscall
	pop r11

	mov r11, 'x' #double space
        push r11
	mov rax, 1
        mov rdi, 1
        mov rsi, rsp
        mov rdx, 1
        syscall
	pop r11

	#Get latest return address
	mov r8, QWORD PTR [rbp + 8] 
	mov rcx, 60 #Count variable

#MAKE A LOOP TO GO THROUGH ALL 16 Bytes in the RA
write_loop:
	push rcx
	push r8
        shr r8, cl
        and r8, 0xf
        mov rdx, 1 #Size t
        lea rsi, [asciis + r8] #convert return address
        mov rdi, 1 # Byte size
        mov rax, 1 # Write syscall
        syscall
	pop r8
	#Decrement count var
	pop rcx
	sub rcx, 4
	cmp rcx, -4
	je update_rbp
	jmp write_loop

