import os
import numpy as np

for filename in os.listdir("../locations/"):
    num_blocks = int(filename.split("_")[0])
    num_requests = int(filename.split("_")[2])
    locations = np.loadtxt("../locations/" + filename, dtype=int)

    problem = """(define (problem pick_place_coordination_problem)
        (:domain pick_place_coordination)
        (:objects
            ur5A ur5B - robot
            home random - state
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
    for blocks in range(num_blocks):
        problem += "block" + chr(blocks + 65) + " "
    problem += "- rail_block\n"

    problem += "\t\t)"

    last_block = "block" + chr(num_blocks + 65 - 1)
    problem += """
        (:init
            (actions-enabled)
            (safe_state home)
            (unsafe_state random)
            (gripper-free ur5A)
            (gripper-free ur5B)
            (robot_config ur5A home)
            (robot_config ur5B home)
            (occupied blockA)
    """

    problem += "\t\t(occupied " + last_block + ")\n"

    for blocks in range(1, num_blocks - 1):
        problem += "\t\t\t(notoccupied block" + chr(blocks + 65) + ")\n"

    problem += "\t\t\t(robot_at ur5A blockA)\n"
    problem += "\t\t\t(robot_at ur5B " + last_block + ")\n"

    problem += "\t\t\t(inside ur5A blockA)\n"
    problem += "\t\t\t(inside ur5B " + last_block + ")\n"

    for requests in range(1, num_requests + 1):
        problem += "\t\t\t(notgrasped ur5A box" + str(requests) + ")\n"

    for requests in range(1, num_requests + 1):
        problem += "\t\t\t(notgrasped ur5B box" + str(requests) + ")\n"

    for blocks in range(num_blocks - 1):
        problem += (
            "\t\t\t(neighbors block"
            + chr(blocks + 65)
            + " block"
            + chr(blocks + 1 + 65)
            + ")\n"
        )
        problem += (
            "\t\t\t(neighbors block"
            + chr(blocks + 1 + 65)
            + " block"
            + chr(blocks + 65)
            + ")\n"
        )

    for requests in range(1, num_requests + 1):
        problem += (
            "\t\t\t(item_at box"
            + str(requests)
            + " box"
            + str(requests)
            + "_pick_loc)\n"
        )

    for requests in range(num_requests):
        problem += (
            "\t\t\t(reachable block"
            + chr(locations[requests, 0] + 65 - 1)
            + " box"
            + str(requests + 1)
            + "_pick_loc)\n"
        )
        problem += (
            "\t\t\t(reachable block"
            + chr(locations[requests, 1] + 65 - 1)
            + " box"
            + str(requests + 1)
            + "_drop_loc)\n"
        )

    problem += "\t\t\t(at 100000 (not (actions-enabled)))\n\t\t)\n"

    problem += "\t\t(:goal (and\n"

    for requests in range(1, num_requests + 1):
        problem += (
            "\t\t\t\t(item_at box"
            + str(requests)
            + " box"
            + str(requests)
            + "_drop_loc)\n"
        )

    problem += "\t\t\t)\n\t\t)\n)"

    with open(
        str(num_blocks) + "_blocks_" + str(num_requests) + "_requests.pddl", "w"
    ) as f:
        f.write(problem)
