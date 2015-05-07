Global	maxOfThree
section	.text
maxOfThree:
	MOV	EAX, [ESP + 4]
	MOV	ECX, [ESP + 8]
	MOV	EDX, [ESP + 12]
	
	CMP	EAX, ECX
	CMOVL	EAX, ECX
	CMP	EAX, EDX
	CMOVL	EAX, EDX
	RET
