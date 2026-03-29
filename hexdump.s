.intel_syntax noprefix
.global _start

#Specifies buffer portion of the text
.section .data
asciis:
	.ascii "0123456789abcdef"
space:
	.byte 0x20

#Specifies next part of the text 
.section .text 

exit_syscall:
        mov rax, 60
        mov rdi, 0 #exit code success
        syscall

_start:
        mov r10, rsp #Must do this in start, otherwise rsp will not contain address of argc
        mov r8, 16 #Count variable, need to skip past file name 

check_argv_loop:
        mov r9, QWORD PTR [r10 + r8] #accessing eight bytes of argv
        cmp r9, 0 #Make sure all bits not null
        je exit_syscall #Exit if 0
        #This is the open syscall
        mov rax, 2
        mov rdi, r9 #Address of the name of the file
        mov rsi, 0
        mov rdx, 0
        syscall
        mov rbx, rax #store the fd of the open file
	mov r15, 0 #Count variable for formatting

byte_rw_loop:
	#Check if counter is multiple of 16 for line counter
	push r15
	and r15, 0x0f #Mask to see if multiple of 16
	cmp r15, 0
	pop r15
	jne cont_loop
	call write_r15
	#TODO: Is it possible to do a conditional call?

cont_loop:
	#This is the read syscall
        #Note that read automatically will increment the byte read in the file
        mov rax, 0
        mov rdi, rbx #Set fd 
        sub rsp, 8 #move stack pointer down one
        mov rsi, rsp #Buffer arg
        mov rdx, 1 #Read 1 byte at a time4
        syscall

        add rsp, 8 #clean up the stack
        cmp rax, 0 #See if end of file reach
        je byte_rw_loop_done

        #Write syscall
	#rdx is already 1 byte for count
        mov rax, 1 #Write syscall
        mov rdi, 1 #Stdout fd
        #rsi is already set to the address of the current byte
        #Conversion to hex
	movzx r12, BYTE PTR [rsi]
	mov r13, r12
	shr r12, 4
	movzx r14, BYTE PTR [asciis + r12] #Get ASCII Character
	mov BYTE PTR [rsi], r14b
	syscall
	#Second ascii character
	and r13, 0x0f #Mask 00001111
	mov rax, 1
	mov rdi, 1
	movzx r14, BYTE PTR [asciis + r13] #Get ASCII Character
	mov BYTE PTR [rsi], r14b
	syscall
	
	#Add space every two characters
        mov r11, ' ' #space
        push r11
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
	mov rdx, 1
        syscall
	pop r11
	#increment counter variable for 1 byte for formatting
	inc r15

	#Check if counter is multiple of 16
	push r15
	and r15, 0x0f #Mask to see if multiple of 16
	cmp r15, 0
	pop r15
	je newline

	#Check if counter is multiple of 8
	push r15
	and r15, 7 #Mask to see if multiple of 8
	cmp r15, 0
	pop r15
	je another_space

	jmp byte_rw_loop

byte_rw_loop_done:
        #Add one more newline
	mov r11, 10 #newline
        push r11
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
        mov rdx, 1
        syscall
        pop r11
	#Close syscall
        mov rax, 3
        mov rdi, rbx #Set fd for file to be closed
        syscall
        add r8, 8  #r8 = r8 + 8
	jmp check_argv_loop

another_space:
	mov r11, ' ' #double space
        push r11
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
	mov rdx, 1
        syscall
        pop r11
	jmp byte_rw_loop

newline:
	mov r11, '\n' #newline
        push r11
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
	mov rdx, 1
        syscall
	pop r11
	#Write syscall
        mov rax, 1 #Write syscall
        mov rdi, 1 #Stdout fd
        mov rdx, 1 #Count        

	jmp byte_rw_loop

write_r15:
	push r15
	shr r15, 28
        and r15, 0xf
        mov rdx, 1 #Size t
        lea rsi,  [asciis + r15] #Access the buffer at the start of the file
        mov rdi, 1
        mov rax, 1
        syscall
        pop r15

	push r15
	shr r15, 24
        and r15, 0xf
        mov rdx, 1 #Size t
        lea rsi,  [asciis + r15] #Access the buffer at the start of the file
        mov rdi, 1
        mov rax, 1
        syscall
        pop r15

	push r15
	shr r15, 20
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15


	push r15
	shr r15, 16
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15


	push r15
	shr r15, 12
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15


	push r15
	shr r15, 8
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15


	push r15
	shr r15, 4
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15


	push r15
	and r15, 0xf
	mov rdx, 1 #Size t
	lea rsi,  [asciis + r15] #Access the buffer at the start of the file
	mov rdi, 1
	mov rax, 1
	syscall
	pop r15

	mov r11, ' ' #double space
        push r11
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
	mov rdx, 1
        syscall
        pop r11

	ret
