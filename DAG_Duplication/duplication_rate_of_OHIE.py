import os


def calculate_duplicate_rate(files):
    total_lines = 0
    unique_lines = set()

    for file_path in files:
        with open(file_path, 'r') as file:
            for line in file:
                total_lines += 1
                unique_lines.add(line.strip())

    if total_lines == 0:
        return 0.0

    rate = len(unique_lines) / total_lines
    return rate


def get_all_files(directory):
    file_list = []

    for root, dirs, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            file_list.append(file_path)

    return file_list


# 要读取的目录路径
directory_path = './block_test'

file_paths = get_all_files(directory_path)
duplicate_rate = calculate_duplicate_rate(file_paths)
print("Effective rate: {:.2%}".format(duplicate_rate))
