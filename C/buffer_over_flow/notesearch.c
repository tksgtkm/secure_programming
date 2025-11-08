#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "notes.h"
#define FILENAME "/var/notes"

/*
メモの出力関数
特定のユーザでオプショナルの検索文字列に適合するメモを表示する関数
ファイルの終端に到達した場合は0、まだメモがある場合は1を返す
*/
int print_notes(int fd, int uid, char *searchstring) {
    int note_length;
    char byte = 0, note_buffer[100];

    note_length = find_user_note(fd, uid);
    // ファイルが終端に到達した場合
    if (note_length == -1) {
        // 0を返す
        return 0;
    }

    // メモのデータを読み込む
    read(fd, note_buffer, note_length);
    // 文字列を終了させる
    note_buffer[note_length] = 0;

    // 検索文字列が見つかった場合
    if (search_note(note_buffer, searchstring)) {
        // メモを出力する
        printf(note_buffer);
    }

    return 1;
}

// 特定ユーザーのメモをファイルから検索する関数
// 特定のuidの次のメモを検索する関数
// ファイルの終端に到達した場合、-1を返す
// そうでない場合、検索されたメモの長さを返す
int find_user_note(int fd, int user_uid) {
    int note_uid = -1;
    unsigned char byte;
    int length;

    // user_uidのメモが検索できる限り繰り返す
    while (note_uid != user_uid) {
        // uidデータを読み込む
        if (read(fd, &note_uid, 4) != 4) {
            // 4バイト読み込めなかった場合、EOFを返す
            return -1;
        }
        // 改行の区切り文字を読み込む
        if (read(fd, &byte, 1) != 1)
            return -1;

        byte = length = 0;
        // 行末までのバイト数を取得する
        while (byte != '\n') {
            // 1バイト読み込む
            if (read(fd, &byte, 1) != 1) {
                // 読み込めなかった場合、EOFコードを返す
                return -1;
            }
            length++;
        }
    }

    // lengthバイトだけファイルを巻き戻す
    lseek(fd, length * -1L, SEEK_CUR);

    printf("[DEBUG] uid %dの%dバイトのメモを見つけました。\n", note_uid, length);

    return length;
}

// キーワード検索関数
// 特定のキーワードに対するメモを検索する関数
// 検索できた場合は1を返し、検索できなかった場合は0を返す
int search_note(char *note, char *keyword) {
    int i, keyword_length, match = 0;

    keyword_length = strlen(keyword);
    // 検索文字列がない場合
    if (keyword_length == 0) {
        // 常に検索できたことにする
        return 1;
    }

    // メモの各バイトごとに繰り返す
    for (i=0; i<strlen(note); i++) {
        // バイトがキーワードと合致した場合
        if (note[i] == keyword[match]) {
            // 次のバイトのチェック準備を行なう
            match++;
        } else {
            // 該当バイトがキーワードの最初のバイトと合致した場合
            if (note[i] == keyword[0]) {
                // 1からマッチングを開始する
                match = 1;
            } else {
                // そうでない場合は0を設定する
                match = 0;
            }
        }
        // 完全に合致した場合
        if (match == keyword_length) {
            // 合致した旨を返す
            return 1;
        }
    }
    // 合致しなかった旨を返す
    return 0;
}

int main(int argc, char *argv[]) {
    // ファイル記述子
    int userid, printing = 1, fd;
    char searchstring[100];
    
    // コマンドライン引数が指定されている場合
    if (argc > 1) {
        // 検索文字列として扱う
        strcpy(searchstring, argv[1]);
    } else {
        // 検索文字列を空に設定する
        searchstring[0] = 0;
    }

    userid = getuid();
    // リードオンリーでファイルをオープンする
    fd = open(FILENAME, O_RDONLY);
    if (fd == -1)
        fatal("read file error");

    while (printing)
        printing = print_notes(fd, userid, searchstring);

    printf("------[ end of memo ]------\n");
    close(fd);
}