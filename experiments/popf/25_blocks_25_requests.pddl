(define (problem pick_place_coordination_problem)
        (:domain pick_place_coordination)
        (:objects
            ur5A ur5B - robot
            home random - state
    		box1 box2 box3 box4 box5 box6 box7 box8 box9 box10 box11 box12 box13 box14 box15 box16 box17 box18 box19 box20 box21 box22 box23 box24 box25 - item
			box1_pick_loc box1_drop_loc - location
			box2_pick_loc box2_drop_loc - location
			box3_pick_loc box3_drop_loc - location
			box4_pick_loc box4_drop_loc - location
			box5_pick_loc box5_drop_loc - location
			box6_pick_loc box6_drop_loc - location
			box7_pick_loc box7_drop_loc - location
			box8_pick_loc box8_drop_loc - location
			box9_pick_loc box9_drop_loc - location
			box10_pick_loc box10_drop_loc - location
			box11_pick_loc box11_drop_loc - location
			box12_pick_loc box12_drop_loc - location
			box13_pick_loc box13_drop_loc - location
			box14_pick_loc box14_drop_loc - location
			box15_pick_loc box15_drop_loc - location
			box16_pick_loc box16_drop_loc - location
			box17_pick_loc box17_drop_loc - location
			box18_pick_loc box18_drop_loc - location
			box19_pick_loc box19_drop_loc - location
			box20_pick_loc box20_drop_loc - location
			box21_pick_loc box21_drop_loc - location
			box22_pick_loc box22_drop_loc - location
			box23_pick_loc box23_drop_loc - location
			box24_pick_loc box24_drop_loc - location
			box25_pick_loc box25_drop_loc - location
			blockA blockB blockC blockD blockE blockF blockG blockH blockI blockJ blockK blockL blockM blockN blockO blockP blockQ blockR blockS blockT blockU blockV blockW blockX blockY - rail_block
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
    		(occupied blockY)
			(notoccupied blockB)
			(notoccupied blockC)
			(notoccupied blockD)
			(notoccupied blockE)
			(notoccupied blockF)
			(notoccupied blockG)
			(notoccupied blockH)
			(notoccupied blockI)
			(notoccupied blockJ)
			(notoccupied blockK)
			(notoccupied blockL)
			(notoccupied blockM)
			(notoccupied blockN)
			(notoccupied blockO)
			(notoccupied blockP)
			(notoccupied blockQ)
			(notoccupied blockR)
			(notoccupied blockS)
			(notoccupied blockT)
			(notoccupied blockU)
			(notoccupied blockV)
			(notoccupied blockW)
			(notoccupied blockX)
			(robot_at ur5A blockA)
			(robot_at ur5B blockY)
			(inside ur5A blockA)
			(inside ur5B blockY)
			(notgrasped ur5A box1)
			(notgrasped ur5A box2)
			(notgrasped ur5A box3)
			(notgrasped ur5A box4)
			(notgrasped ur5A box5)
			(notgrasped ur5A box6)
			(notgrasped ur5A box7)
			(notgrasped ur5A box8)
			(notgrasped ur5A box9)
			(notgrasped ur5A box10)
			(notgrasped ur5A box11)
			(notgrasped ur5A box12)
			(notgrasped ur5A box13)
			(notgrasped ur5A box14)
			(notgrasped ur5A box15)
			(notgrasped ur5A box16)
			(notgrasped ur5A box17)
			(notgrasped ur5A box18)
			(notgrasped ur5A box19)
			(notgrasped ur5A box20)
			(notgrasped ur5A box21)
			(notgrasped ur5A box22)
			(notgrasped ur5A box23)
			(notgrasped ur5A box24)
			(notgrasped ur5A box25)
			(notgrasped ur5B box1)
			(notgrasped ur5B box2)
			(notgrasped ur5B box3)
			(notgrasped ur5B box4)
			(notgrasped ur5B box5)
			(notgrasped ur5B box6)
			(notgrasped ur5B box7)
			(notgrasped ur5B box8)
			(notgrasped ur5B box9)
			(notgrasped ur5B box10)
			(notgrasped ur5B box11)
			(notgrasped ur5B box12)
			(notgrasped ur5B box13)
			(notgrasped ur5B box14)
			(notgrasped ur5B box15)
			(notgrasped ur5B box16)
			(notgrasped ur5B box17)
			(notgrasped ur5B box18)
			(notgrasped ur5B box19)
			(notgrasped ur5B box20)
			(notgrasped ur5B box21)
			(notgrasped ur5B box22)
			(notgrasped ur5B box23)
			(notgrasped ur5B box24)
			(notgrasped ur5B box25)
			(neighbors blockA blockB)
			(neighbors blockB blockA)
			(neighbors blockB blockC)
			(neighbors blockC blockB)
			(neighbors blockC blockD)
			(neighbors blockD blockC)
			(neighbors blockD blockE)
			(neighbors blockE blockD)
			(neighbors blockE blockF)
			(neighbors blockF blockE)
			(neighbors blockF blockG)
			(neighbors blockG blockF)
			(neighbors blockG blockH)
			(neighbors blockH blockG)
			(neighbors blockH blockI)
			(neighbors blockI blockH)
			(neighbors blockI blockJ)
			(neighbors blockJ blockI)
			(neighbors blockJ blockK)
			(neighbors blockK blockJ)
			(neighbors blockK blockL)
			(neighbors blockL blockK)
			(neighbors blockL blockM)
			(neighbors blockM blockL)
			(neighbors blockM blockN)
			(neighbors blockN blockM)
			(neighbors blockN blockO)
			(neighbors blockO blockN)
			(neighbors blockO blockP)
			(neighbors blockP blockO)
			(neighbors blockP blockQ)
			(neighbors blockQ blockP)
			(neighbors blockQ blockR)
			(neighbors blockR blockQ)
			(neighbors blockR blockS)
			(neighbors blockS blockR)
			(neighbors blockS blockT)
			(neighbors blockT blockS)
			(neighbors blockT blockU)
			(neighbors blockU blockT)
			(neighbors blockU blockV)
			(neighbors blockV blockU)
			(neighbors blockV blockW)
			(neighbors blockW blockV)
			(neighbors blockW blockX)
			(neighbors blockX blockW)
			(neighbors blockX blockY)
			(neighbors blockY blockX)
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
			(item_at box21 box21_pick_loc)
			(item_at box22 box22_pick_loc)
			(item_at box23 box23_pick_loc)
			(item_at box24 box24_pick_loc)
			(item_at box25 box25_pick_loc)
			(reachable blockB box1_pick_loc)
			(reachable blockO box1_drop_loc)
			(reachable blockN box2_pick_loc)
			(reachable blockA box2_drop_loc)
			(reachable blockK box3_pick_loc)
			(reachable blockF box3_drop_loc)
			(reachable blockM box4_pick_loc)
			(reachable blockN box4_drop_loc)
			(reachable blockW box5_pick_loc)
			(reachable blockO box5_drop_loc)
			(reachable blockK box6_pick_loc)
			(reachable blockI box6_drop_loc)
			(reachable blockQ box7_pick_loc)
			(reachable blockI box7_drop_loc)
			(reachable blockG box8_pick_loc)
			(reachable blockI box8_drop_loc)
			(reachable blockJ box9_pick_loc)
			(reachable blockR box9_drop_loc)
			(reachable blockJ box10_pick_loc)
			(reachable blockK box10_drop_loc)
			(reachable blockJ box11_pick_loc)
			(reachable blockF box11_drop_loc)
			(reachable blockS box12_pick_loc)
			(reachable blockA box12_drop_loc)
			(reachable blockJ box13_pick_loc)
			(reachable blockF box13_drop_loc)
			(reachable blockD box14_pick_loc)
			(reachable blockM box14_drop_loc)
			(reachable blockO box15_pick_loc)
			(reachable blockQ box15_drop_loc)
			(reachable blockN box16_pick_loc)
			(reachable blockO box16_drop_loc)
			(reachable blockQ box17_pick_loc)
			(reachable blockM box17_drop_loc)
			(reachable blockS box18_pick_loc)
			(reachable blockU box18_drop_loc)
			(reachable blockC box19_pick_loc)
			(reachable blockU box19_drop_loc)
			(reachable blockC box20_pick_loc)
			(reachable blockN box20_drop_loc)
			(reachable blockL box21_pick_loc)
			(reachable blockP box21_drop_loc)
			(reachable blockO box22_pick_loc)
			(reachable blockE box22_drop_loc)
			(reachable blockR box23_pick_loc)
			(reachable blockQ box23_drop_loc)
			(reachable blockM box24_pick_loc)
			(reachable blockQ box24_drop_loc)
			(reachable blockB box25_pick_loc)
			(reachable blockD box25_drop_loc)
			(at 100000 (not (actions-enabled)))
		)
		(:goal (and
				(item_at box1 box1_drop_loc)
				(item_at box2 box2_drop_loc)
				(item_at box3 box3_drop_loc)
				(item_at box4 box4_drop_loc)
				(item_at box5 box5_drop_loc)
				(item_at box6 box6_drop_loc)
				(item_at box7 box7_drop_loc)
				(item_at box8 box8_drop_loc)
				(item_at box9 box9_drop_loc)
				(item_at box10 box10_drop_loc)
				(item_at box11 box11_drop_loc)
				(item_at box12 box12_drop_loc)
				(item_at box13 box13_drop_loc)
				(item_at box14 box14_drop_loc)
				(item_at box15 box15_drop_loc)
				(item_at box16 box16_drop_loc)
				(item_at box17 box17_drop_loc)
				(item_at box18 box18_drop_loc)
				(item_at box19 box19_drop_loc)
				(item_at box20 box20_drop_loc)
				(item_at box21 box21_drop_loc)
				(item_at box22 box22_drop_loc)
				(item_at box23 box23_drop_loc)
				(item_at box24 box24_drop_loc)
				(item_at box25 box25_drop_loc)
			)
		)
)