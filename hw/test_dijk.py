import heapq

def dijkstra(adj, src):
    V = len(adj)

    # Min-heap storing {distance, vertex}
    pq = []

    # Initialize distances to infinity
    dist = [float('inf')] * V

    # Distance from source to itself is 0
    dist[src] = 0
    heapq.heappush(pq, (0, src))

    while pq:

        d, u = heapq.heappop(pq)

        # Skip outdated entries
        if d > dist[u]:
            continue

        # Traverse adjacent vertices
        for x in adj[u]:
            v = x[0]
            weight = x[1]

            # Update distance if a shorter path is found
            if dist[v] > d + weight:
                dist[v] = d + weight
                heapq.heappush(pq, (dist[v], v))

    return dist


if __name__ == "__main__":
    map = {
        "x": 0,
        "z": 1, 
        "y": 2,
        "v": 3,
        "w": 4,
        "u": 5,
        "t": 6
    }
    adj = [
        [[map["z"], 8], [map["y"], 6], [map["v"], 3], [map["w"], 6]], # x
        [[map["x"], 8], [map["y"], 12]], # z
        [[map["x"], 6], [map["z"], 12], [map["t"], 7], [map["v"], 8]], # y
        [[map["x"], 3], [map["w"], 4], [map["u"], 3], [map["t"], 4], [map["y"], 8]], # v
        [[map["x"], 6], [map["v"], 4], [map["u"], 3]], # w
        [[map["w"], 3], [map["v"], 3], [map["t"], 2]], # u
        [[map["u"], 2], [map["v"], 4], [map["y"], 7]] # t
    ]

    src = map["x"]

    dist = dijkstra(adj, src)

    print(dist)