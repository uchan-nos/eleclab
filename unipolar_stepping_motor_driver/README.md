# ユニポーラステッピングモータ制御プログラム

STM32 マイコンを使ったユニポーラタイプのステッピングモータ制御プログラムです。

## ビルド方法

[STM32CubeIDE](https://www.st.com/ja/development-tools/stm32cubeide.html) の最新
バージョンが必要です。

ビルドするには、まず STM32CubeIDE にプロジェクトを追加します。
File > Open Projects from File System... メニューをクリックし、出てきた画面の
Directory... ボタンより unipolar_stepping_motor_driver ディレクトリを開きます。

Folder 欄に unipolar_stepping_motor_driver と表示され、チェックされていることを
確認します。Finish ボタンをクリックすればプロジェクトが追加されます。

コード生成後、Project > Build All メニューをクリックすればビルドされます。

## コード生成

ビルドに必要となるファイルの一部（例えば Drivers ディレクトリ以下のファイル）は
リポジトリに含めてありません。ビルド前にコード生成を行う必要があります。コード生
成を行うには IOC ファイル（unipolar_stepping_motor_driver.ioc）を開いてから
STM32CubeIDE の Project > Generate Code メニューをクリックします。

- IOC ファイルを開くと Generate Code メニューが有効化されます。
- 初回コード生成時は依存パッケージのダウンロードが行われる可能性があります。その
  場合はしばらく時間がかかります。インターネット接続が必要です。
