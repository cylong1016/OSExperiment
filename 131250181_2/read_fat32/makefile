objects = main.c my_print.o

main : $(objects)	# main 是目标文件，依赖 main.c和my_print.o文件生成，下面一行是命令行命令
	gcc -std=c99 -m32 -o main $(objects)
my_print.o : my_print.asm	# my_print.o依赖my_print.asm文件，就是由my_print.asm编译而来
	nasm -f elf -o my_print.o my_print.asm
	
.PHONY : clean
clean :	# 使用make clean命令来删除生成文件,.PHONY意思表示clean是一个“伪目标”，。而在rm命令前面加了一个小减号的意思就是，也许某些文件出现问题，但不要管，继续做后面的事。
	
	-rm main my_print.o
