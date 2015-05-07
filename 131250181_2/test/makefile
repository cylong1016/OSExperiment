objects = Test.c Testnasm.o

Test : $(objects)	# Test 是目标文件，依赖 Test.c和Testnasm.o文件生成，下面一行是命令行命令
	gcc -m32 -o Test $(objects)
Testnasm.o : Test.asm	# Testnasm.o依赖Test.asm文件，就是由Test.asm编译而来
	nasm -f elf -o Testnasm.o Test.asm
	
.PHONY : clean
clean :	# 使用make clean命令来删除中间文件,.PHONY意思表示clean是一个“伪目标”，。而在rm命令前面加了一个小减号的意思就是，也许某些文件出现问题，但不要管，继续做后面的事。
	
	-rm Test Testnasm.o
