import os
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity


def calculate_similarity(folder_paths):
    files = {}
    for folder_path in folder_paths:
        for root, dirs, filenames in os.walk(folder_path):
            for filename in filenames:
                filepath = os.path.join(root, filename)
                name = os.path.splitext(filename)[0]
                if name not in files:
                    files[name] = []
                with open(filepath, 'r') as file:
                    content = file.read()
                    size = os.path.getsize(filepath)
                    files[name].append((content, size))

    vectorizer = TfidfVectorizer()
    similarities_and_sizes = {}
    for name, file_contents in files.items():
        num_files = len(file_contents)
        if num_files > 1:
            tfidf_matrix = vectorizer.fit_transform(
                [content for content, _ in file_contents])
            similarity_matrix = cosine_similarity(tfidf_matrix)
            avg_similarity = similarity_matrix.mean()
            total_size = sum(file_size for _, file_size in file_contents)
            similarities_and_sizes[name] = (avg_similarity, total_size)

    return similarities_and_sizes


# path
folder_paths = []
folder_path1 = "/home/bcno1/Hongyan/Snowball/code/_Transactions_Pool/_127.0.0.1_8001/transactions"
folder_paths.append(folder_path1)
folder_path2 = "/home/bcno1/Hongyan/Snowball/code/_Transactions_Pool/_127.0.0.1_8002/transactions"
folder_paths.append(folder_path2)
folder_path3 = "/home/bcno1/Hongyan/Snowball/code/_Transactions_Pool/_127.0.0.1_8003/transactions"
folder_paths.append(folder_path3)
folder_path4 = "/home/bcno1/Hongyan/Snowball/code/_Transactions_Pool/_127.0.0.1_8004/transactions"
folder_paths.append(folder_path4)

similarities_and_sizes = calculate_similarity(folder_paths)
for name, (similarity, size) in similarities_and_sizes.items():
    print(f"Name: {name}")
    print(f"Similarity: {similarity}")
    print(f"Size: {size} bytes")
