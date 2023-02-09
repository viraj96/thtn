import os
import numpy as np

for filename in os.listdir("../locations/"):
    num_blocks = int(filename.split("_")[0])
    num_requests = int(filename.split("_")[2])
    locations = np.loadtxt("../locations/" + filename, dtype=int)

    problem = """(define (problem pick_place_coordination_problem)
        (:domain pick_place_coordination)
        (:objects
            network - map
            ur5A ur5B - robot
    """

    problem += "\t\t"
    for requests in range(1, num_requests + 1):
        problem += "box" + str(requests) + " "
    problem += "- item\n"

    for requests in range(1, num_requests + 1):
        problem += (
            "\t\t\tbox"
            + str(requests)
            + "_pick_loc box"
            + str(requests)
            + "_drop_loc - location\n"
        )

    problem += "\t\t\t"
    for requests in range(num_requests):
        problem += "request" + chr(requests + 65) + " "
    problem += "- request\n"

    problem += "\t\t\t"
    for blocks in range(num_blocks):
        problem += "block" + chr(blocks + 65) + " "
    problem += "- rail_block\n"

    problem += "\t\t)"

    last_block = "block" + chr(num_blocks + 65 - 1)
    problem += """
        (:object-instants
            ur5A {block = blockA, st = home, grasp = empty}
    """
    problem += "\t\tur5B {block = " + last_block + ", st = home, grasp = empty}\n"

    for requests in range(1, num_requests + 1):
        problem += (
            "\t\t\tbox" + str(requests) + " {loc = box" + str(requests) + "_pick_loc}\n"
        )

    # due_date = 0
    # release_time = 1000
    for requests in range(num_requests):
        problem += (
            "\t\t\trequest"
            + chr(requests + 65)
            + " {release_time = 0, due_date = 10000, demand = [move_item box"
            # + " {release_time = " + str(due_date) + ", due_date = " + str(release_time) + ", demand = [move_item box"
            + str(requests + 1)
            + " box"
            + str(requests + 1)
            + "_drop_loc]}\n"
        )
        # due_date += 1000
        # release_time += 1000

    problem += "\t\t\tnetwork {vertices = ["

    for blocks in range(num_blocks):
        problem += "block" + chr(blocks + 65)
        if blocks != num_blocks - 1:
            problem += " "

    problem += "], edges = ["

    for blocks in range(num_blocks - 1):
        problem += "(block" + chr(blocks + 65) + " block" + chr(blocks + 1 + 65) + ") "
        problem += "(block" + chr(blocks + 1 + 65) + " block" + chr(blocks + 65) + ")"
        if blocks != num_blocks - 1:
            problem += " "

    problem += """
            ]}
        )
        (:htn
            :requests (and\n"""

    for requests in range(num_requests):
        problem += "\t\t\t\t(request" + chr(requests + 65) + ")"
        if requests != num_requests - 1:
            problem += "\n"

    problem += """
            )
        )
        (:init
            (over all (safe_state home))
            (over all (unsafe_state random))\n"""

    for requests in range(num_requests):
        problem += (
            "\t\t\t(over all (reachable block"
            + chr(locations[requests, 0] + 65 - 1)
            + " box"
            + str(requests + 1)
            + "_pick_loc))\n"
        )
        problem += (
            "\t\t\t(over all (reachable block"
            + chr(locations[requests, 1] + 65 - 1)
            + " box"
            + str(requests + 1)
            + "_drop_loc))\n"
        )

    problem += "\t\t)\n)"

    with open(
        str(num_blocks) + "_blocks_" + str(num_requests) + "_requests.hddl", "w"
    ) as f:
        f.write(problem)
