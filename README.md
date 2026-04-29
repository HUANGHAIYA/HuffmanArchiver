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

## 🔧 ビルド方法

```bash
gcc -o huffman_archiver main.c
