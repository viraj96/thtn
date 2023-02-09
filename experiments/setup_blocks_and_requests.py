import numpy as np

blocks = [5, 10, 15, 20, 25]
requests = [5, 10, 15, 20, 25]

for num_blocks in blocks:
    valid_blocks = np.arange(num_blocks)
    valid_blocks = valid_blocks[1:-1]
    for num_requests in requests:
        pick_rb, place_rb = [], []
        for r in range(num_requests):
            pick, place = np.random.choice(valid_blocks, size=2, replace=False)
            pick_rb.append(pick)
            place_rb.append(place)
        pick_rb = np.array(pick_rb, dtype=int)
        place_rb = np.array(place_rb, dtype=int)
        locations = np.hstack((pick_rb[:, None], place_rb[:, None]))
        np.savetxt(
            "locations/"
            + str(num_blocks)
            + "_blocks_"
            + str(num_requests)
            + "_requests.txt",
            locations,
            fmt="%i",
        )
