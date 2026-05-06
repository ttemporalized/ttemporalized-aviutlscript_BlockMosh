# BlockMosh for AviUtl

`BlockMosh` は、AviUtl / 拡張編集向けのブロックグリッチ用スクリプトです。

映像を四角いブロック単位で崩し、データ破損、ブロックノイズ、画面が一瞬だけ壊れるようなモッシュ表現を作れます。

## 収録内容

```text
block_mosh\
  @BlockMosh.obj
  block_mosh_core.dll
```

配布内容によっては、開発用ファイルとして `.c`、`.bat`、`.def`、`.a` が入っている場合があります。通常使用では `@BlockMosh.obj` と `block_mosh_core.dll` を使います。

## インストール

`block_mosh` フォルダを AviUtl の `script` フォルダに入れてください。

```text
C:\aviutl110\script\
  block_mosh\
    @BlockMosh.obj
    block_mosh_core.dll
```

配置後、AviUtl を再起動するかスクリプトを再読み込みします。スクリプト一覧に `@BlockMosh` が表示されれば導入完了です。

## パラメータ

| 項目 | 内容 |
| --- | --- |
| `Map` | 効果の反応タイプ |
| `Mode` | ブロックの崩れ方 |
| `Block` | ブロックサイズ |
| `Intensity` | 崩れの強さ |
| `Carry` | ブロックの流れ具合 |
| `Smear` | 引きずりの量 |
| `Threshold` | 効果が出る範囲 |
| `Drop` | 欠落ブロックの量 |
| `Chroma` | 色ズレの量 |
| `Alpha` | 合成の強さ |
| `Seed` | ランダムの種 |
| `Tint` | 欠落や確認表示に使う色 |
| `Base` | 元映像を残す |
| `Debug` | 確認表示 |
| `Invert` | 反応範囲を反転 |
| `MaskOnly` | 反応範囲だけ表示 |

## 調整の目安

- 細かく壊す: `Block` を小さくする
- 大きく角ばらせる: `Block` を大きくする
- 激しく崩す: `Intensity` と `Carry` を上げる
- 引きずりを増やす: `Smear` を上げる
- 効果範囲を広げる: `Threshold` を下げる
- 色ズレを増やす: `Chroma` を上げる

## 注意

- AviUtl / 拡張編集向けです。
- `@BlockMosh.obj` と `block_mosh_core.dll` は同じフォルダに置いてください。
- DLL を使用するため、環境によっては Windows やセキュリティソフトの確認が入る場合があります。
- 高解像度や重い設定ではプレビューが遅くなることがあります。
- 重要なプロジェクトで使う前にバックアップをおすすめします。
