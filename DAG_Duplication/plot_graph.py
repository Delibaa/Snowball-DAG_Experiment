import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd

if __name__ == '__main__':

    # 读取数据
    data = pd.read_csv('./data/datat1.csv')

    sns.set(style="whitegrid")

    # 设置颜色主题为"muted"
    sns.set_palette("muted")

    # 绘制折线图
    plt.figure(figsize=(10, 6))  # 设置图形大小
    plt.plot(data.iloc[:, 0], data.iloc[:, 1], linewidth=2, label='Column 2')
    plt.plot(data.iloc[:, 0], data.iloc[:, 2], linewidth=2, label='Column 3')
    plt.plot(data.iloc[:, 0], data.iloc[:, 3], linewidth=2, label='Column 4')

    # 添加图例、标题和轴标签
    plt.legend()
    plt.xlabel('Concurrent blocks numbers')
    plt.ylabel('Effective Rate')

    # 显示图形
    plt.show()

