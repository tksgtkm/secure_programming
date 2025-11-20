// 大域変数iを宣言し、123で初期化
volatile int i = 123;
// volatile : C言語で書かれたプログラム以外から値を操作することがあることを示す

int main() {
    i = i + 456;
    return 0;
}