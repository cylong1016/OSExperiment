; 打印 fibonacci 数列，根据用户输入的数字显示相应数量的数列
; 2015-4-15	陈云龙
DATA	SEGMENT					; 定义数据段
PROMPT	DB "Please inpute a number: ", "$"	; 提示语
BUF	DB 128, 0, 128 DUP ("$")
DATA	ENDS					; 数据段结束

STACK	SEGMENT					; 定义栈堆段
	DB 256 DUP(0)
STACK	ENDS

CODE	SEGMENT
	ASSUME CS:CODE, DS:DATA, SS:STACK
START:
	MOV AX, STACK
	MOV SS, AX
	
	; 输出提示语
	MOV AX, DATA
	MOV DS, AX	; 不可以直接 MOV DS, AX
	LEA DX, PROMPT	; 有效地址传送指令 LEA，将有效地址传送到指定的的寄存器，或者(MOV DX, OFFSET PROMPT)
	MOV AH, 9H
	INT 21H		; 通过给AH寄存器赋值9H，然后调用INT 21H指令，显示字符串（DS:DX=串地址，'$'结束字符串）
	
	; 获取输入
	LEA DX, BUF
	MOV AH, 0AH
	INT 21H
	CALL CRLF	; 输出回车换行
	
	; 将输入的多个0-9的数字字符转换成数字
	; 因为缓存区的第一个字符记录的缓存区的最多存储数  
	; 第二个字符是当前输入到缓存区的字符数，所以实际输入字符的偏移地址应该是 BUF[2] 
	LEA DI, BUF[2]
	MOV CL, BUF[1]	; 第二个字符是当前输入到缓冲区的字符数， CX用来计数
	AND CX, 00FFH	; 把高8位变成0,因为BUF[1]中只有8位，放在CX的低8位(CL)中
	CALL STR_TO_INT	; 将用户输入的值转化成数字保存在AX中
	
	; 计算前的准备工作
	MOV CX, AX	; 初始化计数器
	MOV SI, 0	; f(0)低16位
	MOV DI, 0	; f(0)高16位
	MOV AX, 1	; f(1)低16位
	MOV DX, 0	; f(1)高16位
	MOV BL, 00001001B	; 输出的颜色
CALC_LOOP:			; 循环计算
	JCXZ EXIT
	; 输出结果（1 1 2 3 5......）
	XOR BL, 00001111B	; 更换颜色（两种颜色交替）
	CALL PRINT_NUM		; 打印数字
	CALL SPACE		; 输出空格
	; 计算下个值
	PUSH AX;
	PUSH DX;
	ADD AX, SI
	ADC DX, DI		; DX = DI + DX + 上一次计算的进位
	POP DI
	POP SI
	LOOP CALC_LOOP
	
EXIT:	; 安全退出
	MOV AX, 4C00H
	INT 21H		; 其实起作用的就是 AH=4CH，意思就是调用INT 21H的4CH中断，安全退出程序（其实这句等价于MOV AH,4CH INT 21H）
	
;********************** 打印数字 *********************************
PRINT_NUM:	; 最多32位，高16位在DX中，低16位在AX中，BL中存储颜色
	PUSH SI
	PUSH DI
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	MOV SI, BX	; 保存颜色
	MOV CX, 0	; 计算数字的位数
	MOV BX, 10
	MOV DI, AX	; 保存低16位
	
	; 先转化高16位
	MOV AX, DX
LOOP_DIV_H:
	MOV DX, 0
	DIV BX			; 被除数是DX和AX（这就是为什么上一句要吧DX清零的原因），商存入AX，余数存入DX
	CMP AX, 0		; 判断商是否为0
	JE LOOP_DIV_H_END	; 商是0 跳转
	INC CX
	PUSH DX			; 计算出的每一位都放入栈中
	JMP LOOP_DIV_H
LOOP_DIV_H_END:			; 高16位转化结束

	; 再转化低16位
	MOV AX, DI
LOOP_DIV_L:	
	INC CX
	DIV BX
	PUSH DX
	CMP AX, 0		; 判断商是否为0
	JE LOOP_DIV_L_END	; 是0就结束然后输出
	MOV DX, 0
	JMP LOOP_DIV_L		; 循环计算
LOOP_DIV_L_END:			; 低16位转化结束

	MOV DI, CX		; DI为计数器
LOOP_OUT:			; 开始输出
	POP AX			; 从栈中取出每一位输出
	ADD AL, '0'		; 要显示的字符
	MOV BX, SI		; 恢复颜色
	MOV BH, 0		; BH = 显示页
	MOV CX, 1		; CX = 字符显示次数
	
	MOV AH, 09H		; 输出AL中的内容（详情请看 INT 10H 功能）
	INT 10H
	CALL CURSOR_ADD		; 光标后移一位
	DEC DI			; 计数器减1
	JNZ LOOP_OUT		; DI != 0 跳转
	
	POP DX
	POP CX
	POP BX
	POP AX
	POP DI
	POP SI
	RET

;********************** 打印数字 END *********************************

;********************** 光标后移一位 *********************************

CURSOR_ADD:	; 详情请百度INT 10H
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	MOV AH, 03H
	INT 10H		; 调用这个可以获得光标位置
	INC DL		; 光标位置+1
	CMP DL, 79	; 光标位置大于79就换行
	JA LF		; 换行
	MOV AH, 02H
	INT 10H		; 置光标位置
	JMP CURSOR_ADD_END
LF:	
	CALL CRLF
CURSOR_ADD_END:
	POP DX
	POP CX
	POP BX
	POP AX
	ret

;********************** 光标后移 END *********************************

;********************** 输出换行 *********************************
CRLF:
	PUSH AX		; 因为后面用到了AH和DL，所以要把AX和DX保存起来
	PUSH DX
	MOV AH, 02H	; 输出DL中的内容
	
	MOV DL, 0AH
	INT 21H
	MOV DL, 0DH
	INT 21H
	
	POP AX
	POP DX
	RET
;********************** 输出换行 END *********************************

;********************** 输出空格 *********************************
SPACE:
	PUSH AX
	PUSH DX
	MOV DL, ' '
	MOV AH, 02H
	INT 21h
	POP DX
	POP AX
	RET
;********************** 输出空格 END *********************************

;********************** 字符串转化成数字 *********************************	
STR_TO_INT:		; 将DS:[DL]处字符串转化成数字，长度保存在CL（CX的低8位）中，用来计数，转化结果保存到AX中
	PUSH BX;
	PUSH CX;
	PUSH DX;
	
	MOV BX, 10	; BX中保存10,因为乘10进制要 x10
	MOV AX, 0	; 初始化AX，用来保存结果
BEGAN:
	JCXZ LOOP_END
	MUL BX		; 实际上是AX * BX，只不过不需要写AX，规定另一个乘数是AX, 结果32位，高字节存在DX，低字节存在AX中
	MOV DL, DS:[DI]	; 获取缓存中的字符
	SUB DL, '0'
	AND DX, 00FFH	; 将高8位置0
	ADD AX, DX
	INC DI		; 字符串偏移地址加1
	LOOP BEGAN
LOOP_END:
	POP DX
	POP CX
	POP BX
	RET
;********************** 字符串转化成数字 END *********************************

CODE ENDS
END START
