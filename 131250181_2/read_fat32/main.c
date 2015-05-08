/* 2015-5-6 */
/* 陈云龙 */
/* 实验内容：编程读取 FAT12 文件 */
/* 通过分析 fat12 文件系统,打印出所有文件。 */
/* 打印完成后,要求能够获取用户输入文件路径(以回车结束),程序查询Fat12 文件,分别对目录文件、普通文件、不存在的文件进行做相应的输出 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#pragma pack (1)		// 按字节对齐
#define DIR_ITEM_LEN 32	// 文件目录项长度
#define SIZE 1024		// 保存文件名的数组的大小
#define IMG_FILE_NAME "abc.img"

typedef char			byte;	// 字节
typedef unsigned short	word;	// 字
typedef unsigned int	dword;	// 双字

/* FAT12 引导扇区的格式 */
struct BPB {
	word	bytsPerSec;	// 每扇区字节数
	byte	secPerClus;	// 每簇扇区数
	word	rsvdSecCnt;	// Boot 记录占用多少扇区，保留扇区
	byte	numFATs;	// 共有多少 FAT 表
	word	rootEntCnt;	// 目录文件数最大值
	word	totSec16;	// 扇区总数
	byte	media;		// 介质描述符
	word	fATSz16;	// 每 FAT 扇区数
	word	secPerTrk;	// 每磁道扇区数
	word	numHeads;	// 磁头数（面数）
	dword	hiddSec;	// 隐藏扇区数
	dword	totSec32;	// 若 BPB_TotSec16 = 0，则该值记录扇区数
};

/* 根目录区条目 */
struct DIR {
	byte	name[8];	// 文件名 8 字节
	byte	ext[3];		// 扩展名 3 字节
	byte	attr;		// 文件属性，attr = 0x10 表示该文件为一个目录
	byte	resv[10];	// 保留位
	word	wrtTime;	// 最后一次写入时间
	word	wrtDate;	// 最后一次写入日期
	word	fstClus;	// 此条目对应的开始簇号
	dword	fileSize;	// 文件大小
};

FILE* FAT12;
struct BPB bpb;
int	FATStart;	// FAT区起始字节
int DIRStart;	// 根目录区起始字节
int	DATAStart;	// 数据区起始字节
char allPath[SIZE][SIZE] = { '\0' };	// 全部的路径
char* fileData[SIZE];	// 保存文件数据
int pathIdx = 0;

/* 输出字符串，写在my_print.asm中 */
extern void my_print(char* str, int strLen);

void printStr(char* str) {
	my_print(str, strlen(str));
}

/* 打印带颜色的文件名 */
void printFileName(char* parent, char* name, char* ext) {
	printStr(parent);
	strcat(allPath[pathIdx], parent);
	if(strlen(parent)) {
		printStr("/");
		strcat(allPath[pathIdx], "/");
	}
	
	char* nameWithColor = malloc(strlen(name) + 9 + 1 + 3);  // 包含颜色，“.” 和后缀名
	strcpy(nameWithColor, "\e[0;34m");
	strcat(nameWithColor, name);
	strcat(allPath[pathIdx], name);
	if(strlen(ext)) {
		strcat(nameWithColor, ".");
		strcat(allPath[pathIdx], ".");
		strcat(nameWithColor, ext);
		strcat(allPath[pathIdx], ext);
	}
	printStr(nameWithColor);
	printStr("\e[0m\n");
	
	pathIdx++;
}

/* 将文件名末尾的空格去掉 */
void trimFileName(char* name) {
	int maxIdx = 7;
	for(; name[maxIdx] == ' ' && maxIdx >= 0; maxIdx--);
	name[maxIdx + 1] = '\0';
}

/* 将扩展名末尾空格去掉 */
void trimFileExt(char* ext) {
	int maxIdx = 2;
	for(; ext[maxIdx] == ' ' && maxIdx >= 0; maxIdx--);
	ext[maxIdx + 1] = '\0';
}

/* 从FAT12中偏移offset个字节开始读，读取len长度的数据存储到data中 */
void readData(int offset, int len, void* data) {
	fseek(FAT12, offset, SEEK_SET);	// 从偏移offset个字节处开始
	fread(data, 1, len, FAT12);	// 读取len字节到data中
}

/* 找到下一个簇号 */
word findNextClus(word clus) {
	int offset = FATStart + clus * 3 / 2;	// 首簇号在FAT12中的偏移字节
	word temp;
	readData(offset, 2, &temp);	// 读取下一簇
	if(clus % 2) {	// 读取出来的是两个字节，而簇号占用1.5个字节
		clus = temp >> 4;
	} else {
		clus = temp & 0x0FFF;
	}
	return clus;
}

/* 保存文件数据到fileData二维数组中，index和 allPath 二维数组的共用 */
void saveFileData(struct DIR* file) {
	fileData[pathIdx] = malloc(file->fileSize);
	word fstClus = file->fstClus;	// 首簇号
	while(fstClus < 0x0FF8) {
		// 判断坏簇
		if(fstClus >= 0x0FF0 && fstClus <= 0x0FF7) {
			printStr("坏簇！读取失败！\n");
			return;
		}
		// 读取数据区中的数据
		int bytsPerClus = bpb.bytsPerSec * bpb.secPerClus;	// 每簇字节数
		int dataOffset = DATAStart + (fstClus - 2) * bytsPerClus;	// 首簇号对应的数据区在FAT12中的偏移字节
		char* temp = malloc(bytsPerClus);
		readData(dataOffset, bytsPerClus, temp);
		strcat(fileData[pathIdx], temp); // 存储
		fstClus = findNextClus(fstClus); // 找到下一个簇号
	}
	
}

/* 打印文件夹下面的文件 */
void printChildren(struct DIR* root, char* parentPath) {

	word fstClus = root->fstClus;	// 首簇号
	struct DIR subRoot;
	int count = 0;	// 前两个文件名是.和.. 要跳过吧
	
	while (fstClus < 0x0FF8) {
		// 判断坏簇
		if(fstClus >= 0x0FF0 && fstClus <= 0x0FF7) {
			printStr("坏簇！读取失败！\n");
			return;
		}
		
		// 读取数据区中的数据
		int dataOffset = DATAStart + (fstClus - 2) * bpb.bytsPerSec * bpb.secPerClus;	// 首簇号对应的数据区在FAT12中的偏移字节
		int loc = dataOffset;
		for(; loc < dataOffset + bpb.bytsPerSec * bpb.secPerClus; loc += DIR_ITEM_LEN) {
			readData(loc, DIR_ITEM_LEN, &subRoot);	// 读取根目录项
			
			if(subRoot.name[0] == '\xE5')	// 删除的文件
				continue;
			if(subRoot.name[0] == '\0')		// 根目录读取完毕
				break;
			
			count++;
			if(count > 2) {	// 跳过前两个.和..文件
			
				trimFileName(subRoot.name);
				trimFileExt(subRoot.ext);
			
				if(subRoot.attr & 0x10) {		// 目录
					// 复制路径字符串
					char* newPath = malloc(strlen(parentPath) + 13 + 1);
					strcpy(newPath, parentPath);
					strcat(newPath, "/");
					strcat(newPath, subRoot.name);
					printChildren(&subRoot, newPath);
				} else {	// 文件
					// 保存文件数据
					saveFileData(&subRoot);
					// 打印路径
					printFileName(parentPath, subRoot.name, subRoot.ext);
				}
			}
		}
		
		// 找到下一个簇号
		fstClus = findNextClus(fstClus);
	}
	
	if(count <= 2) {
		printFileName(parentPath, subRoot.name, ""); // 这个文件夹是空的，打印这个文件夹名
	}
	
}

/* 根据用户输入显示路径或者文件内容 */
void find(char* input) {
	bool exist = false;
	for(int i = 0; i < SIZE; i++) {
		if(!strcmp(allPath[i], input)) {
			printStr("-----------------\n");
			printStr(fileData[i]);
			printStr("-----------------\n");
			return;
		}
		if(strstr(allPath[i], input) == allPath[i]) {
			exist = true;
			printStr(allPath[i]);
			printStr("\n");
		}
	}
	
	if(!exist) {
		printStr("Unknown file!\n");
	}
}

int main() {
	FAT12 = fopen(IMG_FILE_NAME, "rb");	// 打开FAT12的映像文件
	readData(11, 25, &bpb);		// 读取BPB
	
	// 文件分配表所在的扇区应该是(隐藏扇区+保留扇区)=0+1=第1扇区处
	FATStart = (bpb.hiddSec + bpb.rsvdSecCnt) * bpb.bytsPerSec;
	// 文件目录项最开始位于扇区19(隐藏隐藏+保留扇区+FAT文件分配表占用扇区=0+1+9+9)处
	DIRStart = (bpb.hiddSec + bpb.rsvdSecCnt + bpb.numFATs * bpb.fATSz16) * bpb.bytsPerSec;
	// 数据区起始字节在 DIRStart + 文件目录区大小
	DATAStart = DIRStart + bpb.rootEntCnt * DIR_ITEM_LEN;
	
	struct DIR root;
	int loc = DIRStart;
	for(; loc < DATAStart; loc += DIR_ITEM_LEN) {
		readData(loc, DIR_ITEM_LEN, &root);		// 读取一项根目录项
		
		if(root.name[0] == '\xE5')	// 删除的文件
			continue;
		if(root.name[0] == '\0')	// 根目录读取完毕
			break;
			
		trimFileName(root.name);
		trimFileExt(root.ext);
			
		if(root.attr & 0x10) {		// 目录
			printChildren(&root, root.name);
		} else {	// 文件
			// 保存文件数据
			saveFileData(&root);
			// 打印路径
			printFileName("", root.name, root.ext);
		}
		
	}
	
	while(1) {
		// 处理用户输入
		printStr("<input> ");
		char input[1024];
		scanf("%s", input);
		
		if(!strcmp(input, "quit")) {	// 退出
			break;
		}
		
		int inputLen = strlen(input);
		for (int i = 0; i < inputLen; i++)
			input[i] = toupper(input[i]);	// 转为大写
	
		// 查找
		find(input);
	}
	
	fclose(FAT12);
	return 0;
}

