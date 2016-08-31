#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

//total size of all gameboy memory
#ifndef MEM_SIZE
#define MEM_SIZE 0xFFFF
#endif


//dunno if this is generally true
#ifndef FLASH_SIZE
#define FLASH_SIZE 32768
#endif
#ifndef FLASH_OFFSET
#define FLASH_OFFSET 0x0150
#endif

//size of workable, user-programmable RAM taken from the programming manual
//not a lot here
#ifndef WORKING_RAM_SIZE
#define WORKING_RAM_SIZE (8*1024)
#endif
#ifndef WORKING_RAM_OFFSET
#define WORKING_RAM_OFFSET 0xc000
#endif

typedef struct registers
{
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	int h;
	int l;
	int pc;
	int sp;
}registers;


//registers init
	//this was taken from the gameboy programming manual distributed by nintendo
	//for some reason the stack pointer starts at a ridiculously large initial value.
	//register f is an 8-bit register used for CPU flags, and has the format: ZNHC0000
	//Z denotes zero
	//N denotes subtract	   I don't know
	//H denotes half-carry	   what these mean	
	//C denotes full carry
	//the others are all zero at all times, and are never used
	//the state of f is 0xB0 at init
registers regs={1,0,19,0,0xd8,0xb0,1,0x4d,FLASH_OFFSET,65534};


//convert integer to hexidecimal
//this one uses 2 digit hex (i.e. up to 256)
void to_hex(int num,char* hex){
	if (num>255)
	{
		fprintf(stderr, "number %i is greater than 255!\n", num);
		return;
	}
	hex[2]='\0';
	switch(num/16){
		case 1:
			hex[0]='1';
			break;
		case 2:
			hex[0]='2';
			break;
		case 3:
			hex[0]='3';
			break;
		case 4:
			hex[0]='4';
			break;
		case 5:
			hex[0]='5';
			break;
		case 6:
			hex[0]='6';
			break;
		case 7:
			hex[0]='7';
			break;
		case 8:
			hex[0]='8';
			break;
		case 9:
			hex[0]='9';
			break;
		case 10:
			hex[0]='A';
			break;
		case 11:
			hex[0]='B';
			break;
		case 12:
			hex[0]='C';
			break;
		case 13:
			hex[0]='D';
			break;
		case 14:
			hex[0]='E';
			break;
		case 15:
			hex[0]='F';
			break;
		default:
			hex[0]='0';
			break;
	}
	switch(num%16){
		case 1:
			hex[1]='1';
			break;
		case 2:
			hex[1]='2';
			break;
		case 3:
			hex[1]='3';
			break;
		case 4:
			hex[1]='4';
			break;
		case 5:
			hex[1]='5';
			break;
		case 6:
			hex[1]='6';
			break;
		case 7:
			hex[1]='7';
			break;
		case 8:
			hex[1]='8';
			break;
		case 9:
			hex[1]='9';
			break;
		case 10:
			hex[1]='A';
			break;
		case 11:
			hex[1]='B';
			break;
		case 12:
			hex[1]='C';
			break;
		case 13:
			hex[1]='D';
			break;
		case 14:
			hex[1]='E';
			break;
		case 15:
			hex[1]='F';
			break;
		default:
			hex[1]='0';
			break;
	}
}


//for debugging purposes, it is helpful to be able to dump the registers
void reg_dump(registers regs){
	printf("=====8-BIT REGISTERS=====\n");
	char* hex=(char*)(malloc(3));
	to_hex(regs.a,hex);
	printf("\ta=%i, 0x%s\n",regs.a,hex);
	to_hex(regs.b,hex);
	printf("\tb=%i, 0x%s\n",regs.b,hex);
	to_hex(regs.c,hex);
	printf("\tc=%i, 0x%s\n",regs.c,hex);
	to_hex(regs.d,hex);
	printf("\td=%i, 0x%s\n",regs.d,hex);
	to_hex(regs.e,hex);
	printf("\te=%i, 0x%s\n",regs.e,hex);
	to_hex(regs.f,hex);
	printf("\tf=%i, 0x%s\n",regs.f,hex);
	to_hex(regs.h,hex);
	printf("\th=%i, 0x%s\n",regs.h,hex);
	to_hex(regs.l,hex);
	printf("\tl=%i, 0x%s\n",regs.l,hex);
	printf("=====16-BIT REGISTERS=====\n");
	to_hex(regs.a,hex);
	printf("\taf=%i, 0x%s",256*regs.a+regs.f,hex);
	to_hex(regs.f,hex);
	printf("%s\n",hex);
	to_hex(regs.b,hex);
	printf("\tbc=%i, 0x%s",256*regs.b+regs.c,hex);
	to_hex(regs.c,hex);
	printf("%s\n",hex);
	to_hex(regs.d,hex);
	printf("\tde=%i, 0x%s",256*regs.d+regs.e,hex);
	to_hex(regs.e,hex);
	printf("%s\n",hex);
	to_hex(regs.h,hex);
	printf("\thl=%i, 0x%s",256*regs.h+regs.l,hex);
	to_hex(regs.l,hex);
	printf("%s\n",hex);

	to_hex(regs.pc/256,hex);
	printf("\tpc=%i, 0x%s",regs.pc,hex);
	to_hex(regs.pc%256,hex);
	printf("%s\n",hex);
	to_hex(regs.sp/256,hex);
	printf("\tsp=%i, 0x%s",regs.sp,hex);
	to_hex(regs.sp%256,hex);
	printf("%s\n",hex);
	free(hex);
}

int main(int argc, char** argv){

	//error handling blah blah blah
	if (argc!=2)
	{
		fprintf(stderr, "Usage: myemu <binary_file>\n");
		return -1;
	}

	//open the file
	FILE* fp=fopen(argv[1],"rb");

	//error handling blah blah blah
	if (!fp)
	{
		fprintf(stderr, "File not found: %s\n", argv[1]);
		return -1;
	}


	//2^15 is the size of Tetris (and, I suspect, is also the max size of the data stored in a flash cartridge)
	unsigned char gameboydata[MEM_SIZE];

	//I start by loading the program. This should've been trivial, but it took me forever.
	fread(&gameboydata[0],FLASH_SIZE,1,fp);
	free(fp);

	for (;; ++regs.pc)
	{
		char* opcode=(char*)malloc(3);
		to_hex((int)gameboydata[regs.pc],opcode);
		printf("0x%s encountered\n",opcode);
		switch((int)gameboydata[regs.pc]){
			//dec b
			//decrement b
			//affected flags: Z1H-
			//this one'll be easy
			//oops it's not easy (actually it is, but my to_hex function doesn't work for signed ints)
			//oops wrong again it's hard because cpuflags
			case 0x05:
			{
				printf("decrementing B...\n");
				--regs.b;
				printf("done\nsetting flags...\n");
				if (regs.b==0)
				{
					regs.f|=0x80;
				}
				regs.f|=0x40
				//this is the tricky bit. We now have to set the half-carry bit if and only if 
				//there is a carry performed from bit 3. I have to assume that the bits are
				//zero-indexed, at least for now.
				//I think the best way to do this is to check that the first three bits are one,
				//and the bit in question is zero. (after the decrement)
				if (regs.b&0x0F==0x08)
				{
					regs.f|=0x20;
				}

				//NO, this isn't right. I need to fix it

				printf("done\n");
				break;
			}
			//ld b, d8
			//load immediate 8-bit value into B
			case 0x06:
			{
				int immediate=(int)gameboydata[regs.pc+1];
				char* hex=(char*)malloc(3);
				to_hex(immediate,hex);
				printf("loading immediate value %i, 0x%s into B\n",immediate,hex);
				regs.b=immediate;
				++regs.pc;
				free(hex);
				printf("done\n");
				break;
			}
			//dec c
			//decrement c
			//I hate cpuflags
			case 0x0D:
			{
				printf("decrementing B...\n");
				if (regs.c%16==0)
				{
					regs.f|=0x20;
				}
				--regs.c;
				printf("done\nsetting flags...\n");

				//so the way I think this works is if b becomes 0, you set the Z flag
				//if you need to carry from the first hex digit of b, you set the H flag
				//since it decrements, the N flag is always set.


				//need to find a way to take ABCD0000 -> A'1C'D0000
				//now I need to take ABCD0000 -> A1CD0000
				regs.f|=0x40;
				if (regs.c==0)
				{
					regs.f|=0x80;
				}
				else{
					regs.f&=0x7f;
				}
				printf("done\n");
				break;
			}
			//ld c, d8
			//load immediate 8-bit value into C
			case 0x0E:
			{	
				int immediate=(int)gameboydata[regs.pc+1];
				char* hex=(char*)malloc(3);
				to_hex(immediate,hex);
				printf("loading immediate value %i, 0x%s into C\n",immediate,hex);
				regs.c=immediate;
				++regs.pc;
				free(hex);
				printf("done\n");
				break;
			}
			//jp NZ, r8
			//change the PC by an 8-bit signed displacement if NZ (not Zero) is true, or continue
			case 0x20:
			{
				printf("checking f flags...\n");
				if (regs.f&0x80)
				{
					printf("done and continuing\n");
					++regs.pc;
					break;
				}
				printf("done\n");
				char* hex=(char*)malloc(3);
				to_hex((int)gameboydata[regs.pc+1],hex);
				int offset=(int)gameboydata[regs.pc+1];
				if ((int)gameboydata[regs.pc+1]>127)
				{
					printf("%i\n", offset);
					offset^=0xFF;
					printf("%i\n", offset);
					--offset;
					offset=-offset;
				}
				printf("jumping by %d, 0x%s\n",offset,hex);
				regs.pc+=offset-1;
				free(hex);
				printf("done\n");
				break;
			}
			//ld hl d16
			//load immediate 16-bit value into HL
			case 0x21:
			{
				int immediate=(int)gameboydata[regs.pc+1]+(int)gameboydata[regs.pc+2]*256;
				printf("loading immediate value %i into HL...\n", immediate);
				regs.h=immediate/256;
				regs.l=immediate%256;
				regs.pc+=2;
				printf("done\n");
				break;
			}

			//ldd (hl),a
			//load the value stored in A into the address pointed to by HL and decrement HL
			case 0x32:
			{
				int addr=regs.h*256+regs.l;
				char* hex=(char*)malloc(3);
				to_hex(regs.a,hex);
				printf("storing %i, 0x%s to address %i\n",regs.a,hex,addr);
				free(hex);
				gameboydata[addr]=(unsigned char)regs.a;
				printf("decrementing hl...\n");
				if (regs.l==0)
				{
					regs.l=0xff;
					if (regs.h==0)
					{
						regs.h=0xff;
					}
					else{
						--regs.h;
					}
				}
				else{
					--regs.l;
				}
				printf("done\n");
				break;
			}
			//ld h,c
			/*case 0x61:
			{
				break;
			}*/
			//XOR A
			//xor a with itself
			//affected flags: Z000
			case 0xaf:
			{
				printf("XOR'ing A\n");
				regs.a^=regs.a;
				printf("setting F\n");
				if (regs.a==0)
				{
					regs.f|=0x80;
				}
				else{
					regs.f&=0x7f;
				}
				printf("done\n");
				break;
			}
			//jp a16
			//jump to a 16-bit address
			//I'm guessing that it uses the next two bytes as the address, so let's roll with that for now
			//update: two bytes confirminatti
			case 0xC3:
			{
				char* hex=(char*)malloc(3);
				to_hex(regs.pc/256,hex);
				printf("old addr= %d, 0x%s",regs.pc, hex);
				to_hex(regs.pc%256,hex);
				printf("%s\n", hex);
				unsigned int new_addr=(int)gameboydata[regs.pc+1];
				new_addr+=((int)gameboydata[regs.pc+2])*256;
				regs.pc=new_addr-1;
				to_hex(regs.pc/256,hex);
				printf("new addr= %d, 0x%s",regs.pc,hex);
				to_hex(regs.pc%256,hex);
				printf("%s\n",hex);
				free(hex);
				break;
			}
			default:
				char* hex=(char*)malloc(3);
				to_hex(regs.pc/256,hex);
				fprintf(stderr, "Instruction not recognized at %d, 0x%s",regs.pc,hex);
				to_hex(regs.pc%256,hex);
				printf("%s: %i, 0x%s\n",hex,(int)gameboydata[regs.pc],opcode);
				free(hex);
				free(opcode);
				reg_dump(regs);
				return 1;
		}
		free(opcode);
	}
	return 0;
}