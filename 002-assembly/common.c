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

void print_arr_int (int arr[], int len)
{
    for (int i = 0; i < len; ++i) {
        printf ("%d  ", arr[i]);
    }

    printf ("\n");
}
