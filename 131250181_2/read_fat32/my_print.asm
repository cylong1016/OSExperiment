global my_print

section .text
my_print:
	MOV	EAX, 4
	MOV	EBX, 1
	MOV	ECX, [ESP + 4]	; 字符串首地址
	MOV	EDX, [ESP + 8]	; 字符串长度
	INT	80H
	ret
	
