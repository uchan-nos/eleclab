# 各プロジェクトのビルド方法

1. [ch32v003fun](https://github.com/cnlohr/ch32v003fun) が必要です。
2. ./env.mk に ch32v003fun へのパスを設定します。
3. 各プロジェクトのディレクトリに移動して make します。

# ファイル構成

## env.mk

各種の環境変数を設定します。代表的な環境変数は次の通りです。

- CH32V003FUN
  - ch32v003fun へのパスを指定します。
  - $(CH32V003FUN)/ch32fun/ch32fun.mk が存在すれば正しいパスになっているはずです。
- LIBC_DIR
  - libc.a が置いてあるディレクトリへのパスを指定します。
    - 実際には LIBC_DIR の後にマイコン名（ch32v203 など）が結合されて正しいパスとなるようにします。
    - 下記の手順で Newlib をビルドした場合、LIBC_DIR=/path/to/newlib-cygwin/newlib-build_ とします。

## {proj}/Makefile

プロジェクト固有の Makefile です。
典型的には $(CH32V003FUN)/ch32fun/ch32fun.mk を include するようになっています。

## 99-minichlink.rules

WCH-LinkE を一般ユーザーで使えるようにする Udev ルールです。
/etc/udev/rules.d にリンクします。

    $ sudo ln -s $PWD/99-minichlink.rules /etc/udev/rules.d/

# Newlib のビルド

一部のプログラムは ch32v003fun に含まれない標準ライブラリの機能を利用するため、Newlib を必要とします。
Newlib は以下の手順でビルドします。

    $ git clone git://sourceware.org/git/newlib-cygwin.git
    $ cd newlib-cygwin/newlib/
    $ mkdir ../newlib-build_ch32v203
    $ cd ../newlib-build_ch32v203
    $ ../newlib/configure --host=riscv64-unknown-elf CFLAGS="-I../newlib/libc/machine/riscv -msmall-data-limit=8 -march=rv32imac -mabi=ilp32"
    $ make（詳細なコマンドラインを表示したい場合は $ V=1 make とする）

Newlib をビルドする際のコンパイラオプションは、Newlib を利用する側のオプションとそろえる必要があります。
そのため、上記の手順では、newlib-build_ch32v203 のように使用するマイコンごとにディレクトリを分け、
それぞれのオプションでビルドするようにしています。

コンパイラオプション `-msmall-data-limit=8 -march=rv32imac -mabi=ilp32` の部分はマイコンにより変わります。
各プログラムを make すればコンパイラオプションが画面に表示されますので、それを使えば良いです。

make でエラーが出ず、newlib-build_ch32v203 以下に libc.a や libm.a が生成されれば成功です。

## host と build と target

./configure には `--host` と `--build` と `--target` というオプションがあります。
それぞれの説明は [warning: using cross tools not prefixed with host triplet の意味 - ぷるぷるの雑記](https://prupru-prune.hatenablog.com/entry/2023/09/02/185429) が分かりやすいです。
この記事によれば、Newlib における `--host` は、その Newlib が実際に動作する環境です。
今回は CH32V 用のターゲットトリプルである riscv64-unknown-elf を指定します。
`--build` は、その Newlib をビルドする環境です。
指定しない場合、configure を実行した環境（つまりパソコン）が自動的に指定されます。
Newlib において `--target` は意味を持たず、通常は指定しません。

上記のビルド手順の `--host=riscv64-unknown-elf` は、riscv64-unknown-elf 上で動く Newlib を作るように指示しているわけです。
