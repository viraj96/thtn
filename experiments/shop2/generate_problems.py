import os
import numpy as np

for filename in os.listdir("../locations/"):
    num_blocks = int(filename.split("_")[0])
    num_requests = int(filename.split("_")[2])
    locations = np.loadtxt("../locations/" + filename, dtype=int)

    problem = """(in-package :shop-user)
    """
    problem += """(defdomain pick_place_coordination (

        (:operator (!move_to_home_state ?r ?target ?current ?time)
            (
                (domain_time ?time)
                (safe_state ?target)
                (unsafe_state ?current)
                (robot_config ?r ?current)
                (assign ?new_time (+ ?time 10))
            )
            (
                (domain_time ?time)
                (robot_config ?r ?current)
            )
            (
                (domain_time ?new_time)
                (robot_config ?r ?target)
            )
        )

        (:operator (!rail_move ?r ?home ?from ?to ?time)
            (
                (occupied ?from)
                (safe_state ?home)
                (domain_time ?time)
                (robot_at ?r ?from)
                (not (occupied ?to))
                (neighbors ?from ?to)
                (robot_config ?r ?home)
                (assign ?new_time (+ ?time 20))
            )
            (
                (occupied ?from)
                (robot_at ?r ?from)
                (domain_time ?time)
            )
            (
                (occupied ?to)
                (robot_at ?r ?to)
                (domain_time ?new_time)
            )
        )

        (:operator (!grasp ?r ?i ?home ?target ?from ?iloc ?time)
            (
                (gripper_free ?r)
                (item_at ?i ?iloc)
                (safe_state ?home)
                (robot_at ?r ?from)
                (domain_time ?time)
                (not (grasped ?r ?i))
                (unsafe_state ?target)
                (reachable ?from ?iloc)
                (robot_config ?r ?home)
                (assign ?new_time (+ ?time 30))
            )
            (
                (gripper_free ?r)
                (item_at ?i ?iloc)
                (domain_time ?time)
                (robot_config ?r ?home)
            )
            (
                (grasped ?r ?i)
                (gripper_used ?r)
                (domain_time ?new_time)
                (robot_config ?r ?target)
            )
        )

        (:operator (!release ?r ?i ?home ?target ?from ?iloc ?time)
            (
                (grasped ?r ?i)
                (gripper_used ?r)
                (safe_state ?home)
                (domain_time ?time)
                (robot_at ?r ?from)
                (unsafe_state ?target)
                (reachable ?from ?iloc)
                (robot_config ?r ?home)
                (assign ?new_time (+ ?time 30))
            )
            (
                (grasped ?r ?i)
                (gripper_used ?r)
                (domain_time ?time)
                (robot_config ?r ?home)
            )
            (
                (gripper_free ?r)
                (item_at ?i ?iloc)
                (domain_time ?new_time)
                (robot_config ?r ?target)
            )
        )

        (:method (print-current-state)
            ((eval (print-current-state)))
            ()
        )

        (:method (pick_item ?r ?i)
            (
                (safe_state ?home)
                (item_at ?i ?iloc)
                (robot_at ?r ?from)
                (not (grasped ?r ?i))
                (robot_config ?r ?home)
                (reachable ?from ?iloc)
            )
            (:ordered
                (
                    (!grasp ?r ?i ?home ?target ?from ?iloc ?time_one)
                    (!move_to_home_state ?r ?home ?target ?time_two)
                )

            )
        )

        (:method (drop_item ?r ?i ?drop_loc)
            (
                (grasped ?r ?i)
                (gripper_used ?r)
                (safe_state ?home)
                (robot_at ?r ?from)
                (robot_config ?r ?home)
                (reachable ?from ?drop_loc)
            )
            (:ordered
                (
                    (!release ?r ?i ?home ?target ?from ?drop_loc ?time_one)
                    (!move_to_home_state ?r ?home ?target ?time_two)
                )
            )
        )

        (:method (get_to ?r ?dest)
            (
                (robot_at ?r ?dest)
            )
            (:ordered
                ()
            )
        )

        (:method (get_to ?r ?dest_rb)
            (
                (robot_at ?r ?current)
                (neighbors ?current ?n1)
                (distance ?n1 ?dest_rb ?d1)
                (distance ?current ?dest_rb ?d2)
                (call < ?d1 ?d2)
                (not (neighbors ?current ?dest_rb))
            )
            (:ordered
                (
                    (get_to ?r ?n1)
                    (get_to ?r ?dest_rb)
                )
            )
        )

        (:method (get_to ?r ?dest_rb)
            (
                (robot_at ?r ?current)
                (not (occupied ?dest_rb))
                (neighbors ?current ?dest_rb)
            )
            (:ordered
                (
                    (!rail_move ?r ?home ?current ?dest_rb ?time)
                )
            )
        )

        (:method (get_to ?r1 ?dest_rb)
            (
                (occupied ?dest_rb)
                (robot_at ?r1 ?current)
                (robot_at ?r2 ?dest_rb)
                (neighbors ?current ?dest_rb)
                (neighbors ?dest_rb ?next_rb)
                (distance ?current ?next_rb ?d)
                (call > ?d 0)
            )
            (:ordered
                (
                    (!rail_move ?r2 ?home ?dest_rb ?next_rb ?time_one)
                    (!rail_move ?r1 ?home ?current ?dest_rb ?time_two)
                )
            )
        )

        (:method (move_item ?i ?drop_loc)
            (
                (safe_state ?home)
                (not (grasped ?r ?i))
                (item_at ?i ?pick_loc)
                (robot_config ?r ?home)
                (reachable ?pick_rb ?pick_loc)
                (reachable ?drop_rb ?drop_loc)
            )
            (:ordered
                (
                    (get_to ?r ?pick_rb)
                    (pick_item ?r ?i)
                    (get_to ?r ?drop_rb)
                    (drop_item ?r ?i ?drop_loc)
                )
            )
        )

    ))

    (defproblem task pick_place_coordination
        (
            (domain_time 0)
            (gripper_free ur5A)
            (gripper-free ur5B)
            (safe_state home)
            (unsafe_state rand)
            (robot_config ur5A home)
            (robot_config ur5B home)
            (occupied blockA)
    """

    last_block = "block" + chr(num_blocks + 65 - 2)
    problem += "\t\t(occupied " + last_block + ")\n"

    problem += "\t\t\t(robot_at ur5A blockA)\n"
    problem += "\t\t\t(robot_at ur5B " + last_block + ")\n"

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

    for blocks_outer in range(num_blocks):
        for blocks_inner in range(num_blocks):
            problem += (
                "\t\t\t(distance block"
                + chr(blocks_outer + 65)
                + " block"
                + chr(blocks_inner + 65)
                + " "
                + str(abs(blocks_outer - blocks_inner))
                + ")\n"
            )
        problem += "\n"

    problem += "\t\t)\n\t\t(\n"

    for requests in range(1, num_requests + 1):
        problem += (
            "\t\t\t(move_item box"
            + str(requests)
            + " box"
            + str(requests)
            + "_drop_loc)\n"
        )

    problem += "\t\t)\n)\n"

    problem += "(find-plans 'task :verbose :long-plans)"

    with open(
        str(num_blocks) + "_blocks_" + str(num_requests) + "_requests.lisp", "w"
    ) as f:
        f.write(problem)
