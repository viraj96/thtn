import os
import json
import numpy as np

data = {}
for filename in os.listdir("../experiments/locations/"):
    num_blocks = int(filename.split("_")[0])
    num_requests = int(filename.split("_")[2])
    locations = np.loadtxt("../experiments/locations/" + filename, dtype=int)
    data["block_bounds"] = {}
    data["locations"] = {}
    start = -1.25
    step = 2.50 / num_blocks
    for blocks in range(num_blocks):
        b = "block" + chr(blocks + 65)
        lb = start + step * blocks
        ub = lb + step
        data["block_bounds"][b] = [lb, ub]
    for requests in range(num_requests):
        item = "box" + str(requests + 1)
        pick_b = "block" + chr(locations[requests, 0] + 65 - 1)
        drop_b = "block" + chr(locations[requests, 1] + 65 - 1)
        data["locations"][item + "_pick_loc"] = [0.5, sum(data["block_bounds"][pick_b]) / 2, 1.03]
        data["locations"][item + "_drop_loc"] = [0.5, sum(data["block_bounds"][drop_b]) / 2, 1.03]

    with open(str(num_blocks) + "_blocks_" + str(num_requests) + "_requests.json", "w") as f:
        json.dump(data, f)
