#ifndef __VM_H
#define __VM_H

#include <stdint.h>

#ifndef __ASSEMBLY__
typedef unsigned long pfn_t;
#endif

#define CONSOLE_write 0
#define CONSOLE_read  1

/*
ゲストドメイン(Dom0, DomU)が起動される際の初期メモリマップの定義
x86アーキテクチャにおけるカーネルイメージ、initrd、ページテーブルなど
各構成要素がどのように仮想メモリ上に並ぶかを規定している

* `incontents 200 startofday 開始時のメモリレイアウト
 *
 *  1. ドメイン（仮想マシン）は、連続した仮想メモリ領域内で起動される。
 *  2. この連続領域は、4MB境界に揃って終了する。
 *  3. 初期の仮想メモリ領域におけるブートストラップ要素の配置順序は以下の通り：
 *      a. 再配置されたカーネルイメージ
 *      b. 初期RAMディスク              [mod_start, mod_len]
 *         （省略される場合もある）
 *      c. 割り当てられたページフレームのリスト [mfn_list, nr_pages]
 *      d. start_info_t 構造体          [レジスタ rSI（x86）]
 *         dom0（最初のドメイン）の場合、このページにはコンソール情報も含まれる
 *      e. dom0 でない場合: リングページ
 *      f. dom0 でない場合: コンソールリングページ
 *      g. ブートストラップページテーブル [pt_base および CR3（x86）]
 *      h. ブートストラップスタック       [レジスタ ESP（x86）]
 *  4. ブートストラップ要素は互いに詰めて配置されるが、各要素は4kB境界に揃えられる。
 *  5. ページフレームのリストは、ドメインのための連続した「疑似物理」メモリ
 *     レイアウトを形成する。特に、ブートストラップ仮想メモリ領域は
 *     疑似物理マップの最初のセクションに対して 1:1 で対応している。
 *  6. すべてのブートストラップ要素はゲストOSから読み書き可能にマップされる。
 *     唯一の例外はブートストラップページテーブルで、これは読み取り専用でマップされる。
 *  7. 最後のブートストラップ要素の後には、少なくとも512kBのパディングが保証される。
 *     必要であれば、このパディングを確保するために仮想領域が追加の4MB分拡張される。
*/
struct start_info {
    char magic[32];
    unsigned long nr_pages;
    unsigned long shared_info;
    uint32_t flags;
    pfn_t store_mfn;
    uint32_t store_evtchn;
    union {
        struct {
            pfn_t mfn;
            uint32_t evtchn;
        } domU;
        struct {
            uint32_t info_off;
            uint32_t info_size;
        } dom0;
    } console;
    unsigned long pt_base;
    unsigned long nr_pt_frames;
    unsigned long mfn_list;
    unsigned long mod_start;
    unsigned long mod_len;
};
typedef struct start_info start_info_t;

#endif