#include <stdio.h>
/*
int_pointerが文字データを指し、char_pointerが整数データを指す
*/
int main() {
    int i;

    char char_array[5] = {'a', 'b', 'c', 'd', 'e'};
    int int_array[5] = {1, 2, 3, 4, 5};

    char *char_pointer;
    int *int_pointer;

    // char_pointerとint_pointerが整合性のない
    // データ型のアドレスを指し示すようにする
    char_pointer = int_array;
    int_pointer = char_array;

    // int_pointerを繰り返し用いて整数の配列要素を取得
    for (i = 0; i < 5; i++) {
        printf("[整数へのポインタ]は%pを指しており、その内容は'%c'です。\n",
        int_pointer, *int_pointer);
        int_pointer = int_pointer + 1;
    }

    // char_pointerを繰り返し用いて文字の配列要素を取得
    for (i = 0; i < 5; i++) {
        printf("[文字へのポインタ]は%pを指しており、その内容は%dです。\n",
        char_pointer, *char_pointer);
        char_pointer = char_pointer + 1;
    }
}

/*
gcc poiner_type2.c を実行するとコンパイラから警告が出力される
$ gcc pointer_types2.c 
pointer_types2.c: In function ‘main’:
pointer_types2.c:14:18: warning: assignment to ‘char *’ from incompatible pointer type ‘int *’ [-Wincompatible-pointer-types]
   14 |     char_pointer = int_array;
      |                  ^
pointer_types2.c:15:17: warning: assignment to ‘int *’ from incompatible pointer type ‘char *’ [-Wincompatible-pointer-types]
   15 |     int_pointer = char_array;
      |                 ^

コンパイラはポインタに整合性のない型の値が格納されているメモリアドレスを代入しようとしている
警告を発し、プログラマに対して過ちの可能性があることを教えてくれる。

[整数へのポインタ]は0x7ffcf8a25e13を指しており、その内容は'a'です。
[整数へのポインタ]は0x7ffcf8a25e17を指しており、その内容は'e'です。
[整数へのポインタ]は0x7ffcf8a25e1bを指しており、その内容は'�'です。
[整数へのポインタ]は0x7ffcf8a25e1fを指しており、その内容は'�'です。
[整数へのポインタ]は0x7ffcf8a25e23を指しており、その内容は'�'です。
[文字へのポインタ]は0x7ffcf8a25df0を指しており、その内容は1です。
[文字へのポインタ]は0x7ffcf8a25df1を指しており、その内容は0です。
[文字へのポインタ]は0x7ffcf8a25df2を指しており、その内容は0です。
[文字へのポインタ]は0x7ffcf8a25df3を指しており、その内容は0です。
[文字へのポインタ]は0x7ffcf8a25df4を指しており、その内容は2です。

int_pointerが指している先には5バイトの文字データがあるものの、
このポインタは依然として整数型の値を指し示すものとして機能する。
このポインタに1を加算するとアドレスは4ずつ増えることになる。
一方、char_pointerに1を加算すると、20バイトの整数データ(4バイトの整数が5つ)
を指しているアドレスは1つずつ増えることになる。
この4バイトの整数を1バイトずつ参照していく過程でも整数データのバイト順が
リトルエンディアンになっていることが確認できる。
0x00000001という4バイトの値はメモリ上では実際のところ0x01 0x00 0x00 0x00 という順序で
格納されている。

これはポインタに整合性のない型の値が格納されたアドレスを代入すると発生する。
ポインタはそれが指し示しているデータを取得する際、そのポインタ自体に定義されている
型を用いてデータの型を決定するため、型を正しくしておくことは重要になる。
*/