# サポートページ

xxxのサポートページです．当面はREADME.mdで書きますが，ボリュームが大きくなったらGitHub Pagesで書きたい．

## 環境構築

### 環境

以下の環境で動作確認済みです．

### VirtualBox

### Vagrant

## 4_algorithms

このレポジトリをクローンしてください．

### WireSharkによる実験

WireSharkの`vagrant`ディレクトリに移動し，`vagrant up`してください．

```bash
cd tcp-book/4_algorithms/wireshark/vagrant
vagrant up
```

シェルを二つ起動し，`guest1`にSSH接続します．`jupyter notebook`を`localhost`で使うため，以下のオプションで接続してください．

```bash
vagrant ssh -- -L 7777:localhost:7777
```

１つ目のシェルで`wireshark`を起動します．

```bash
sudo wireshark
```

２つ目のシェルで，100MBの`tempfile`を`guest2`に転送します．

```bash
ftp -n < src/4_algorithms/wireshark/ftp_conf.txt
```

### ns-3

WireSharkの`vagrant`ディレクトリに移動し，`vagrant up`してください．

```bash
cd tcp-book/4_algorithms/ns3/vagrant
vagrant up
```

## 正誤表

## About us

[特定非営利活動法人 neko 9 Laboratories](https://www.neko9.org/)は，情報通信および情報処理に関する研究開発事業を通じて，学術の振興に寄与することを目的としています．

（著者一覧的なものを書きたい）

info[at]neko9.org
