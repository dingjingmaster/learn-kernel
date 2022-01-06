/*************************************************************************
> FileName: common.c
> Author  : DingJing
> Mail    : dingjing@live.cn
> Created Time: 2022年01月05日 星期三 14时58分09秒
 ************************************************************************/
#include <stdio.h>

void print_hello ()
{
    printf ("hello\n");
}

void print_enter ()
{
    printf ("\n");
}

void print_short (short i)
{
    printf ("%d\n", i);
}

void print_int (int i)
{
    printf ("%d\n", i);
}

void print_float (float f)
{
    printf ("%f\n", f);
}

void print_long (long i)
{
    printf ("%ld\n", i);
}

void print_arr_int (int arr[], int len)
{
    for (int i = 0; i < len; ++i) {
        printf ("%d  ", arr[i]);
    }

    printf ("\n");
}
