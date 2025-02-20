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

## {proj}/Makefile

プロジェクト固有の Makefile です。
典型的には $(CH32V003FUN)/ch32fun/ch32fun.mk を include するようになっています。
