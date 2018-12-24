import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from tqdm import tqdm

sns.set_style(style='ticks')

# 計算結果を出力するディレクトリ名．
output_path = 'data/chapter4/'
# 作図結果を保存するディレクトリ名．
save_path = '/home/vagrant/shared/'
# TCPアルゴリズム一覧．
algorithms = [
    'TcpNewReno', 'TcpHybla', 'TcpHighSpeed', 'TcpHtcp',
    'TcpVegas', 'TcpScalable', 'TcpVeno', 'TcpBic', 'TcpYeah',
    'TcpIllinois', 'TcpWestwood', 'TcpLedbat']


def make_command(
    algorithm, prefix_name, duration
    bandwidth=None, delay=None,
    access_bandwidth=None, access_delay=None,
    data=None, mtu=None, num_flows=None, flow_monitor=None,
    pcap_tracing=None):

    """
    コマンドライン引数を追加したコマンドを作成する関数
    - algorithm: 輻輳制御アルゴリズム名．
    - prefix_name: 出力するファイルのプレフィックス名．pwdからの相対パスで表す．
    - duration: シミュレーション時間[s]．
    - bandwidth: ボトルネック部分の帯域．例：'2Mbps'
    - delay: ボトルネック部分の遅延．例：'0.01ms'
    - access_bandwidth: アクセス部分の帯域．例:'10Mbps'
    - access_delay: アクセス部分の遅延．例:'45ms'．
    - data: 送信するデータ総量[MB]
    """

    cmd = './waf --run "chapter4-base'
    cmd += ' --transport_prot={}'.format(algorithm)
    cmd += ' --prefix_name={}'.format(prefix_name)
    cmd += ' --tracing=True'
    cmd += ' --duration={}'.format(duration)

    if bandwidth:
        cmd += ' --bandwidth={}'.format(bandwidth)
    if delay:
        cmd += ' --delay={}'.format(delay)
    if access_bandwidth:
        cmd += ' --access_bandwidth={}'.format(access_bandwidth)
    if access_delay:
        cmd += ' --access_delay={}'.format(access_delay)
    cmd += '"'

    return cmd


def read_data(prefix_name, metric, duration):
    """
    {prefix_name}{metric}.dataを読みだす関数
    """

    file_name = '{}{}.data'.format(prefix_name, metric)
    data = pd.read_table(
        file_name, names=['sec', 'value'], delimiter=' ')
    data = data[data.sec <= duration].reset_index(
        drop=True)

    # 作画用に，最終行にduration秒のデータを追加．
    if duration > data.sec.max():
        tail = data.tail(1)
        tail.sec = duration
        data = pd.concat([data, tail])
    return data


def plot_metric(
    metric, x_max, y_label, y_deno=1, x_ticks=False):

    """
    metricの時系列変化をプロットする関数．
    y_denoは単位変換に用いる（byte->segment）
    """

    plt.step(
        metric.sec, metric.value/y_deno,
        c='k', where='pre')
    plt.xlim(0, x_max)
    plt.ylabel(y_label)

    # x軸のメモリを表示するか否か．
    if x_ticks:
        plt.xlabel('time[s]')
    else:
        plt.xticks([])


def plot_cong_state(
    cong_state, x_max, y_label, x_ticks=False):
    """
    cong_stateの時系列変化をプロットする関数．
    """

    # 2:rcwは今回の分析対象外なので，
    # 3，4を一つ前にずらす．
    new_state = {
        0:0 , 1:1, 3:2, 4:3}

    # 最初はOpen状態．
    plt.fill_between(
        [0, x_max],
        [0, 0],
        [1, 1],
        facecolor='gray')

    # 各輻輳状態ごとに該当秒数を塗りつぶす．
    for target_state in range(4):
        for sec, state in cong_state.values:
            if new_state[state]==target_state:
                color = 'gray'
            else:
                color = 'white'

            plt.fill_between(
                [sec, x_max],
                [target_state, target_state],
                [target_state+1, target_state+1],
                facecolor=color)

    # 各服装状態を区切る横線を描画．
    for i in range(1, 4):
        plt.plot([0, x_max], [i, i], 'k-')

    plt.xlim(0, x_max)
    plt.ylim(0, 4)
    plt.yticks(
        [0.5, 1.5, 2.5, 3.5],
        ['open', 'disorder', 'recovery', 'loss'])
    plt.ylabel(y_label)

    # x軸のメモリを表示するか否か．
    if x_ticks:
        plt.xlabel('time[s]')
    else:
        plt.xticks([])


def plot_algorithm(algo, duration, save_path):
    path = '{}{}/'.format(save_path, algo)
    """
    algorithmのcwnd，ssth，ack，rtt，cong-stateをプロットする関数．
    """

    mss=340

    # データの読み込み
    cwnd = read_data(path, 'cwnd', duration)
    ssth = read_data(path, 'ssth', duration)
    ack = read_data(path, 'ack', duration)
    rtt = read_data(path, 'rtt', duration)
    cong_state = read_data(path, 'cong-state', duration)

    # 描画．
    plt.figure(figsize=(12, 12))
    plt.subplot(5, 1, 1)
    plot_metric(cwnd, duration, 'cwnd[segment]', mss)
    plt.subplot(5, 1, 2)
    plot_metric(ssth, duration, 'ssth[segment]', mss)
    plt.subplot(5, 1, 3)
    plot_metric(ack, duration, 'ack[segment]', mss)
    plt.subplot(5, 1, 4)
    plot_metric(rtt, duration, 'rtt[s]')
    plt.subplot(5, 1, 5)

    # 一番下のプロットのみx軸を描画．
    plot_cong_state(
        cong_state, duration, 'cong-state',
        x_ticks=True)

    # 保存
    plt.savefig('{}{}_duration_{}s.png'.format(
        save_path, algo.lower(), duration))


def execute_and_plot(
    algorithm, duration, save_path,
    bandwidth=None, delay=None,
    access_bandwidth=None, access_delay=None,
    data=None, mtu=None, num_flows=None, flow_monitor=None,
    pcap_tracing=None):

    path = '{}{}/'.format(save_path, algorithm)
    if not os.path.exists(path):
        os.mkdir(path)

    cmd = make_command(
        algorithm=algorithm, duration=duration,
        prefix_name=path)

    plot_algorithm(algorithm, duration, save_path)
