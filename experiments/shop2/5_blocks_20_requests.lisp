(in-package :shop-user)
    (defdomain pick_place_coordination (

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
    		(occupied blockD)
			(robot_at ur5A blockA)
			(robot_at ur5B blockD)
			(neighbors blockA blockB)
			(neighbors blockB blockA)
			(neighbors blockB blockC)
			(neighbors blockC blockB)
			(neighbors blockC blockD)
			(neighbors blockD blockC)
			(neighbors blockD blockE)
			(neighbors blockE blockD)
			(item_at box1 box1_pick_loc)
			(item_at box2 box2_pick_loc)
			(item_at box3 box3_pick_loc)
			(item_at box4 box4_pick_loc)
			(item_at box5 box5_pick_loc)
			(item_at box6 box6_pick_loc)
			(item_at box7 box7_pick_loc)
			(item_at box8 box8_pick_loc)
			(item_at box9 box9_pick_loc)
			(item_at box10 box10_pick_loc)
			(item_at box11 box11_pick_loc)
			(item_at box12 box12_pick_loc)
			(item_at box13 box13_pick_loc)
			(item_at box14 box14_pick_loc)
			(item_at box15 box15_pick_loc)
			(item_at box16 box16_pick_loc)
			(item_at box17 box17_pick_loc)
			(item_at box18 box18_pick_loc)
			(item_at box19 box19_pick_loc)
			(item_at box20 box20_pick_loc)
			(reachable blockB box1_pick_loc)
			(reachable blockA box1_drop_loc)
			(reachable blockC box2_pick_loc)
			(reachable blockB box2_drop_loc)
			(reachable blockB box3_pick_loc)
			(reachable blockC box3_drop_loc)
			(reachable blockC box4_pick_loc)
			(reachable blockB box4_drop_loc)
			(reachable blockB box5_pick_loc)
			(reachable blockC box5_drop_loc)
			(reachable blockB box6_pick_loc)
			(reachable blockA box6_drop_loc)
			(reachable blockC box7_pick_loc)
			(reachable blockA box7_drop_loc)
			(reachable blockA box8_pick_loc)
			(reachable blockB box8_drop_loc)
			(reachable blockB box9_pick_loc)
			(reachable blockA box9_drop_loc)
			(reachable blockB box10_pick_loc)
			(reachable blockC box10_drop_loc)
			(reachable blockA box11_pick_loc)
			(reachable blockB box11_drop_loc)
			(reachable blockB box12_pick_loc)
			(reachable blockC box12_drop_loc)
			(reachable blockC box13_pick_loc)
			(reachable blockA box13_drop_loc)
			(reachable blockB box14_pick_loc)
			(reachable blockA box14_drop_loc)
			(reachable blockB box15_pick_loc)
			(reachable blockA box15_drop_loc)
			(reachable blockB box16_pick_loc)
			(reachable blockA box16_drop_loc)
			(reachable blockA box17_pick_loc)
			(reachable blockB box17_drop_loc)
			(reachable blockC box18_pick_loc)
			(reachable blockA box18_drop_loc)
			(reachable blockB box19_pick_loc)
			(reachable blockA box19_drop_loc)
			(reachable blockA box20_pick_loc)
			(reachable blockB box20_drop_loc)
			(distance blockA blockA 0)
			(distance blockA blockB 1)
			(distance blockA blockC 2)
			(distance blockA blockD 3)
			(distance blockA blockE 4)

			(distance blockB blockA 1)
			(distance blockB blockB 0)
			(distance blockB blockC 1)
			(distance blockB blockD 2)
			(distance blockB blockE 3)

			(distance blockC blockA 2)
			(distance blockC blockB 1)
			(distance blockC blockC 0)
			(distance blockC blockD 1)
			(distance blockC blockE 2)

			(distance blockD blockA 3)
			(distance blockD blockB 2)
			(distance blockD blockC 1)
			(distance blockD blockD 0)
			(distance blockD blockE 1)

			(distance blockE blockA 4)
			(distance blockE blockB 3)
			(distance blockE blockC 2)
			(distance blockE blockD 1)
			(distance blockE blockE 0)

		)
		(
			(move_item box1 box1_drop_loc)
			(move_item box2 box2_drop_loc)
			(move_item box3 box3_drop_loc)
			(move_item box4 box4_drop_loc)
			(move_item box5 box5_drop_loc)
			(move_item box6 box6_drop_loc)
			(move_item box7 box7_drop_loc)
			(move_item box8 box8_drop_loc)
			(move_item box9 box9_drop_loc)
			(move_item box10 box10_drop_loc)
			(move_item box11 box11_drop_loc)
			(move_item box12 box12_drop_loc)
			(move_item box13 box13_drop_loc)
			(move_item box14 box14_drop_loc)
			(move_item box15 box15_drop_loc)
			(move_item box16 box16_drop_loc)
			(move_item box17 box17_drop_loc)
			(move_item box18 box18_drop_loc)
			(move_item box19 box19_drop_loc)
			(move_item box20 box20_drop_loc)
		)
)
(find-plans 'task :verbose :long-plans)