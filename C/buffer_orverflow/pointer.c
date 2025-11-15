#include <stdio.h>
#include <string.h>

int main() {
    // 20個の要素を持つ文字の配列
    char str_a[20];
    // 文字の配列を指すポインタ
    char *pointer;
    char *pointer2;
    
    strcpy(str_a, "Hello World\n");
    // 1つ目のポインタが配列の先頭を指すように設定する
    pointer = str_a;
    // 1つ目のポインタが指している文字列を表示する
    printf(pointer);

    // 2つ目のポインタは2バイト先を指すように設定する
    pointer2 = pointer + 2;
    // 2つ目のポインタが指している文字列をコピーする
    printf(pointer2);
    // その場所に他の文字列をコピーする
    strcpy(pointer2, "y you guys!\n");
    // 1つ目のポインタが指している文字列を表示する
    printf(pointer);
}