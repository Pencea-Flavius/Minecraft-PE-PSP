#include "util/fast_memcpy.h"
#include <string.h>

typedef unsigned char  u8;
typedef unsigned int   u32;

void memcpy_vfpu( void* dst, const void* src, size_t size )
{

    if( !vfpuCopyWorthIt( (u32)src, (u32)dst, size ) )
    {
        memcpy( dst, src, size );
        return;
    }

    u8* src8 = (u8*)src;
    u8* dst8 = (u8*)dst;

	while( ((u32)dst8&0x3)!=0 )
	{
		*dst8++ = *src8++;
		size--;
	}

	u32 *dst32=(u32*)dst8;
	u32 *src32=(u32*)src8;

	while( ((u32)dst32&0xF)!=0 )
	{
		*dst32++ = *src32++;
		size -= 4;
	}

	dst8=(u8*)dst32;
	src8=(u8*)src32;

	if( ((u32)src8&0xF)==0 )
	{
		while (size>63)
		{
			asm(".set	push\n"
				".set	noreorder\n"
				"lv.q c000, 0(%1)\n"
				"lv.q c010, 16(%1)\n"
				"lv.q c020, 32(%1)\n"
				"lv.q c030, 48(%1)\n"
				"sv.q c000, 0(%0)\n"
				"sv.q c010, 16(%0)\n"
				"sv.q c020, 32(%0)\n"
				"sv.q c030, 48(%0)\n"
				"addiu  %2, %2, -64\n"
				"addiu	%1, %1, 64\n"
				"addiu	%0, %0, 64\n"
				".set	pop\n"
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}

		while (size>15)
		{
			asm(".set	push\n"
				".set	noreorder\n"
				"lv.q c000, 0(%1)\n"
				"sv.q c000, 0(%0)\n"
				"addiu  %2, %2, -16\n"
				"addiu	%1, %1, 16\n"
				"addiu	%0, %0, 16\n"
				".set	pop\n"
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}
	}
	else
    {

		while (size>63)
		{
			asm(".set	push\n"
				".set	noreorder\n"
				"ulv.q c000, 0(%1)\n"
				"ulv.q c010, 16(%1)\n"
				"ulv.q c020, 32(%1)\n"
				"ulv.q c030, 48(%1)\n"
				"sv.q c000, 0(%0)\n"
				"sv.q c010, 16(%0)\n"
				"sv.q c020, 32(%0)\n"
				"sv.q c030, 48(%0)\n"
				"addiu  %2, %2, -64\n"
				"addiu	%1, %1, 64\n"
				"addiu	%0, %0, 64\n"
				".set	pop\n"
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory","$f0","$f1","$f2","$f3"
				);
		}

		while (size>15)
		{
			asm(".set	push\n"
				".set	noreorder\n"
				"ulv.q c000, 0(%1)\n"
				"sv.q c000, 0(%0)\n"
				"addiu  %2, %2, -16\n"
				"addiu	%1, %1, 16\n"
				"addiu	%0, %0, 16\n"
				".set	pop\n"
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory","$f0","$f1","$f2","$f3"
				);
		}
    }

	if (size == 0)
		return;

	dst32=(u32*)dst8;
	src32=(u32*)src8;

	while( size>3 )
	{
		*dst32++ = *src32++;
		size -= 4;
	}

	dst8=(u8*)dst32;
	src8=(u8*)src32;

	while( size>0 )
    {
        *dst8++ = *src8++;
        size--;
    }
}
