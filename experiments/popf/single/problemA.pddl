(define (problem pick_place_coordination_problem)
        (:domain pick_place_coordination_rosplan)
        (:objects
            box1 - item
            ur5A ur5B - robot
            home random - state
			box1_pick_loc box1_drop_loc - location
			blockA blockB blockC blockD blockE - rail_block
		)
        (:init
            (actions-enabled)
            (safe_state home)
            (unsafe_state random)
            (gripper-free ur5A)
            (gripper-free ur5B)
            (robot_config ur5A home)
            (robot_config ur5B home)
            (occupied blockA)
    		(occupied blockB)
			(notoccupied blockC)
			(notoccupied blockD)
			(notoccupied blockE)
			(robot_at ur5A blockA)
			(robot_at ur5B blockB)
			(inside ur5A blockA)
			(inside ur5B blockB)
			(notgrasped ur5A box1)
			(notgrasped ur5B box1)
			(neighbors blockA blockB)
			(neighbors blockB blockA)
			(neighbors blockB blockC)
			(neighbors blockC blockB)
			(neighbors blockC blockD)
			(neighbors blockD blockC)
			(neighbors blockD blockE)
			(neighbors blockE blockD)
			(item_at box1 box1_pick_loc)
			(reachable blockA box1_pick_loc)
			(reachable blockB box1_drop_loc)
			(at 100000 (not (actions-enabled)))
		)
		(:goal (and
				(item_at box1 box1_drop_loc)
			)
		)
)
