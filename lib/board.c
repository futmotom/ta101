/*
 * Copyright (C) 2005 Texas Instruments.
 *
 * (C) Copyright 2004
 * Jian Zhang, Texas Instruments, jzhang@ti.com.
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <part.h>
#include <fat.h>
#include <mmc.h>

#ifdef CONFIG_NEWERA4430ES2
#ifdef CFG_DDR_TEST
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))
#define __raw_writel(v,a)		__arch_putl(v,a)

#define writel(v,a)			(*(unsigned int *)(a) = (v))
#define readl(a)			(*(unsigned int *)(a))

#define LPDDR2_START	0x80000000
#define LPDDR2_END	0x9FFFFFFF

#define puts	serial_puts
#define putc	serial_putc

void * memcpy(void * dest,const void *src,size_t count)
{
	u32 pcount=0;
	char *tmp = (char *) dest, *s = (char *) src;
	while (count--) {
		*tmp++ = *s++;
		pcount += 4;
		if (pcount == 16*1024*1024) {
			pcount=0;
			putc('*');
		}
	}
	return dest;
}
#endif /* CFG_DDR_TEST */
#endif /* CONFIG_NEWERA4430ES2 */

#ifdef CFG_PRINTF
int print_info(void)
{
	printf ("\n\nTexas Instruments X-Loader 1.41 ("
		__DATE__ " - " __TIME__ ")\n");
	return 0;
}
#endif
typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
	cpu_init,		/* basic cpu dependent setup */
	board_init,		/* basic board dependent setup */
#ifdef CFG_PRINTF
#ifndef CFG_DDR_TEST
 	serial_init,		/* serial communications setup */
#endif /* CFG_DDR_TEST */
	print_info,
#endif
   	//nand_init,		/* board specific nand init */
	NULL,
};

#ifdef CONFIG_NEWERA4430ES2
#ifdef CFG_DDR_TEST
int ddr_test(void)
{
	unsigned char i,ret = 0,pass_total=0,fail_total=0;
	u32 j=1,k,readvalue,count=0,failcount1=0;
	u32 testaddr,testvalue,tmp=0,failcount2=0;
	u32 SDRAM_memcpytest_length,mem_src,mem_dst,failcount3=0,failcount4=0;
	printf ("\n\nFlextronics DDR RAM TEST 1.00 ("
			__DATE__ " - " __TIME__ ")\n");
	__raw_writel(WD_UNLOCK1, WD2_BASE + WSPR);
	wait_for_command_complete(WD2_BASE);
	__raw_writel(WD_UNLOCK2, WD2_BASE + WSPR);

	//**********************************************************************
	// walking test - This test walks 1's and 0's to test for
	// simultaneous switching noise.
	//**********************************************************************

	while (j <= CFG_DDR_TEST ) {
		testaddr=LPDDR2_START;
		printf("\n<DDR RAM TEST %d>\n\nDDR walk 1 test...\n",j);
#ifdef CFG_DDR_TEST_VERBOSE
		printf("Testing address : 0x%8x  Value : 0x",testaddr);
		printf("\033[?25l");
#else
		printf("Testing start : 0x%8x   end : 0x%8x\n",testaddr,LPDDR2_END);
#endif

		while (testaddr <= LPDDR2_END) {
			// walking 1's test
			testvalue=0x80000000;

			for (i=0;i<32;i++) {
#ifdef CFG_DDR_TEST_VERBOSE
				printf("%8x\b\b\b\b\b\b\b\b",testvalue);
#endif
				writel(testvalue,testaddr);
				readvalue=readl(testaddr);
				if (testvalue != readvalue) {
					printf("\nRam test failed at 0x%8x\n",testaddr);
					printf("Write Value : 0x%8x\n",testvalue);
					printf("Read  Value : 0x%8x\n",readvalue);
					failcount1++;
				}
				testvalue = testvalue >> 1;
			}
			testaddr=testaddr+4;

#ifdef CFG_DDR_TEST_VERBOSE
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			printf("%8x  Value : 0x",testaddr);
#else
			count += 4;
			if (count == 4*1024*1024) {
				count=0;
				putc('*');
			}
#endif
		}
		if (failcount1 == 0) {
			puts("Passed!\n");
		} else {
			ret = 1 ;
			printf("\nDDR walk 1 test failed %d times. \n",failcount1);
		}
		testaddr=LPDDR2_START;
		puts("\nDDR walk 0 test...\n");

#ifdef CFG_DDR_TEST_VERBOSE
		printf("Testing address : 0x%8x  Value : 0x",testaddr);
		printf("\033[?25l");
#else
		printf("Testing start : 0x%8x   end : 0x%8x\n",testaddr,LPDDR2_END);
#endif

		while(testaddr <= LPDDR2_END) {
			// walking 0's test
			testvalue=0xFFFFFFFF;
			tmp=0x80000000;

			for (i=0;i<32;i++) {
				testvalue = testvalue & (~tmp);
#ifdef CFG_DDR_TEST_VERBOSE
				printf("%8x\b\b\b\b\b\b\b\b",testvalue);
#endif
				writel(testvalue,testaddr);
				readvalue=readl(testaddr);
				if (testvalue != readvalue) {
					printf("\nRam test failed at 0x%8x\n",testaddr);
					printf("Write Value : 0x%8x\n",testvalue);
					printf("Read  Value : 0x%8x\n",readvalue);
					failcount2++;
				}
				testvalue=testvalue | tmp;
				tmp = tmp >>1;
			}
			testaddr += 4;
#ifdef CFG_DDR_TEST_VERBOSE
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			printf("%8x  Value : 0x",testaddr);
#else

			count += 4;
			if (count == 4*1024*1024) {
				count=0;
				putc('*');
			}
#endif
		}
		testaddr=LPDDR2_START;
		if (failcount2 == 0) {
			puts("Passed!\n");
		} else {
			ret = 1 ;
			printf("\nDDR walk 0 test failed %d times. \n",failcount2);
		}

//**********************************************************************
// DDR single access and memcpy test -  This test fills the DDR source
// with a pattern and checks it then performs a memcpy from the source
// to the destination and checks the destination.
//**********************************************************************

		SDRAM_memcpytest_length = (LPDDR2_END - LPDDR2_START+1)/2;
		mem_src	= LPDDR2_START;
		mem_dst	= LPDDR2_START + SDRAM_memcpytest_length;
		puts("\nDDR single access and memcpy test...\n");
		puts("Filling memory source...\n");
		// fill memory source
		for( testaddr = mem_src, k = 0, testvalue = 0; k < SDRAM_memcpytest_length ; k += 4, testaddr += 4 ) {
			writel(testvalue,testaddr);
			testvalue += 0x12345678;
			// print progress indicator every 16MB
			count += 4;
			if (count == 16*1024*1024) {
				count=0;
				putc('*');
			}
		}
		puts("Done!\nMemory copying...\n");
		/* Perform memcpy */
		memcpy((unsigned int *)mem_dst, (unsigned int *)mem_src, SDRAM_memcpytest_length);
		// verify values were transferred
		puts("Done!\nMemory verifying...\n");
		for( testaddr = mem_dst, k = 0, testvalue = 0; k < SDRAM_memcpytest_length; k += 4, testaddr += 4 ) {
			tmp=readl(testaddr);
			if ( tmp != testvalue) {
				failcount3++;
				printf("Address = 0x%x\n", testaddr);
				printf("Data read was:   0x%x\n", tmp);
				printf("But pattern was: 0x%x\n", testvalue);
			}
			testvalue += 0x12345678;
			// print progress indicator every 16MB
			count += 4;
			if (count == 16*1024*1024) {
				count=0;
				putc('*');
			}
		}
		puts("Done!\n");
		if (failcount3 != 0) {
			ret = 1 ;
			puts("**FAILED : Memory didn't transfer correctly, memcpy failed\n");
			puts("  checking source to see if it is correct \n");
			// verify values were programmed in the source
			for( testaddr = mem_src, k = 0, testvalue = 0; k < SDRAM_memcpytest_length; k += 4, testaddr += 4 ) {
				tmp=readl(testaddr);
				if (tmp != testvalue) {
					failcount4++;
				}
				testvalue += 0x12345678;
				count += 4;
				if (count == 16*1024*1024) {
					count=0;
					putc('*');
				}

			}
			if (failcount4 != 0)
				puts("**\nFAILED: Memory source wasn't programmed correctly\n");
			else
				puts("\nMemory source was programmed correctly.\n");
		}
		else
			puts("\nMemory was transfered correctly, memcpy passed.\n");
		if (ret) {
			fail_total++;
			printf("\n<DDR RAM TEST FAILED %d !>\n",j);
		} else {
			pass_total++;
			printf("\n<DDR RAM TEST PASSED %d !>\n",j);
		}
		j++;
	}
#ifdef CFG_DDR_TEST_VERBOSE
	printf("\033[?25h");
#endif
	printf("\n Tested %d times    Passed %d times    Failed %d times",j,pass_total,fail_total);
	return ret;
}
#endif /* CFG_DDR_TEST */
#endif /* CONFIG_NEWERA4430ES2 */

#ifdef CFG_CMD_FAT
extern char * strcpy(char * dest,const char *src);
#else
char * strcpy(char * dest,const char *src)
{
	 char *tmp = dest;

	 while ((*dest++ = *src++) != '\0')
	         /* nothing */;
	 return tmp;
}
#endif

#ifdef CFG_CMD_MMC
int mmc_read_bootloader(int dev, int part)
{
	unsigned char ret = 0;
	unsigned long offset = CFG_LOADADDR;

	ret = mmc_init(dev);
	if (ret != 0) {
		printf("\n MMC init failed \n");
		return -1;
	}

#ifdef CFG_CMD_FAT
	long size;
	block_dev_desc_t *dev_desc = NULL;

	if (part) {
		dev_desc = mmc_get_dev(dev);
		fat_register_device(dev_desc, part);
		size = file_fat_read("u-boot.bin", (unsigned char *)offset, 0);
		if (size == -1)
			return -1;
	} else {
		/* FIXME: OMAP4 specific */
		 mmc_read(dev, 0x400, (unsigned char *)CFG_LOADADDR,
							0x00060000);
	}
#endif
	return 0;
}
#endif

extern int do_load_serial_bin(ulong offset, int baudrate);

#define __raw_readl(a)	(*(volatile unsigned int *)(a))

void start_armboot (void)
{
  	init_fnc_t **init_fnc_ptr;
 	int i;
	uchar *buf;
	char boot_dev_name[8];
	u32 boot_device = 0;
 
   	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}
#ifdef START_LOADB_DOWNLOAD
	strcpy(boot_dev_name, "UART");
	do_load_serial_bin (CFG_LOADADDR, 115200);
#else

	/* Read boot device from saved scratch pad */
	boot_device = __raw_readl(0x4A326000) & 0xff;
	buf = (uchar *) CFG_LOADADDR;

	switch(boot_device) {
	case 0x03:
		strcpy(boot_dev_name, "ONENAND");
#if defined(CFG_ONENAND)
		for (i = ONENAND_START_BLOCK; i < ONENAND_END_BLOCK; i++) {
			if (!onenand_read_block(buf, i))
				buf += ONENAND_BLOCK_SIZE;
			else
				goto error;
		}
#endif
		break;
	case 0x02:
	default:
		strcpy(boot_dev_name, "NAND");
#if defined(CFG_NAND)
		for (i = NAND_UBOOT_START; i < NAND_UBOOT_END;
				i+= NAND_BLOCK_SIZE) {
			if (!nand_read_block(buf, i))
				buf += NAND_BLOCK_SIZE; /* advance buf ptr */
		}
#endif
		break;
	case 0x05:
		strcpy(boot_dev_name, "MMC/SD1");
#if defined(CONFIG_MMC)
		if (mmc_read_bootloader(0, 1) != 0)
			goto error;
#endif
		break;
	case 0x06:
		strcpy(boot_dev_name, "EMMC");
#if defined(CONFIG_MMC)
		if (mmc_read_bootloader(1, 0) != 0)
			goto error;
#endif
		break;
	};
#endif
	/* go run U-Boot and never return */
	printf("Starting OS Bootloader from %s ...\n", boot_dev_name);
 	((init_fnc_t *)CFG_LOADADDR)();

	/* should never come here */
#if defined(CFG_ONENAND) || defined(CONFIG_MMC)
error:
#endif
	printf("Could not read bootloader!\n");
	hang();
}

void hang (void)
{
	/* call board specific hang function */
	board_hang();
	
	/* if board_hang() returns, hange here */
	printf("X-Loader hangs\n");
	for (;;);
}
