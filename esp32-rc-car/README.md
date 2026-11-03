## ビルド方法

Arduino IDE でビルドする前に、以下の 2 つのシンボリックリンクを controller-esp32c6, receiver-esp32c6 それぞれに作成してください。

- esp32rc.cpp --> ../esp32rc.cpp
- esp32rc.h   --> ../esp32rc.h

Windows では管理者権限のある PowerShell で次のコマンドを使うことでシンボリックリンクを作成できます。

    New-Item -ItemType SymbolicLink -Path esp32rc.cpp -Target ..\esp32rc.cpp
    New-Item -ItemType SymbolicLink -Path esp32rc.h -Target ..\esp32rc.h
