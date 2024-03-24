import numpy as np

queries = np.load("queries.npy")
data = np.load("data.npy")

queries_shape = queries.shape
data_shape = data.shape

print("Queries shape = ", queries_shape)
print("Data shape = ", data_shape)

dist = np.zeros((data_shape[0], queries_shape[0]))
for i in range(data_shape[0]):
    for j in range(queries_shape[0]):
        query = queries[j, :]
        dat = data[i, :]
        dist[i, j] = np.linalg.norm(query - dat)

dist = np.reshape(dist, (data_shape[0] * queries_shape[0],))
print(dist)
