# HuffmanArchiver
本プロジェクトは、C言語で実装したファイル圧縮CLIツールです。
Huffman符号化を用いて複数ファイルを1つのアーカイブとして圧縮・展開できます。

---

## 機能

- create : 複数ファイルを圧縮してアーカイブ作成  
- extract : アーカイブの解凍  
- list : アーカイブ内のファイル一覧表示  
- test : 圧縮率の検証  

---

## ビルド方法

```bash
gcc -o compress src/main.c
```
## 実行方法
コンパイル後、ターミナルで以下のコマンドとオプションが使用可能です
```bash
Usage: ./compress <command> [options]
Commands:
  create <archive> <files...>   - Create compressed archive from files
  extract <archive> [directory] - Extract archive to directory
  list <archive>                - List contents of archive
  test <file>                   - Analyze compression efficiency
```
