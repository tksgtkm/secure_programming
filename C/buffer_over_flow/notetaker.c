#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "notes.h"

void usage(char *prog_name, char *filename) {
    printf("usage: %s <add data to %s>\n", prog_name, filename);
    exit(0);
}

int main(int argc, char *argv[]) {
    // ファイル記述子
    int userid, fd;
    char *buffer, *datafile;

    buffer = (char *)ec_malloc(100);
    datafile = (char *)ec_malloc(20);
    strcpy(datafile, "/var/notes");

    // コマンドライン引数が与えられていない場合
    if (argc < 2) {
        // 使用方法を表示して終了
        usage(argv[0], datafile);
    }

    // コマンドライン引数をバッファにコピーする
    strcpy(buffer, argv[1]);

    printf("[DEBUG] budder   @ %p: \'%s\'\n", buffer, buffer);
    printf("[DEBUG] datafile @ %p: \'%s\'\n", datafile, datafile);

    // ファイルのオープン
    fd = open(datafile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1)
        fatal("error occured to open file in main()");
    printf("[DEBUG] file description: %d\n", fd);

    // 実ユーザーIDを取得する
    userid = getuid();

    // データの書き込み
    // メモの前にユーザIDを書き込む
    if (write(fd, &userid, 4) == -1) {
        fatal("error occured for write userid to file in main()");
    }
    // 改行する
    write(fd, "\n", 1);

    // メモを書き込む
    if (write(fd, buffer, strlen(buffer) == -1))
        fatal("error occured for write buffer to file in main()");
    // 改行する
    write(fd, "\n", 1);

    // ファイルのクローズ
    if (close(fd) == -1)
        fatal("error occured closing file in main()");

    printf("saved memo\n");
    free(buffer);
    free(datafile);
}