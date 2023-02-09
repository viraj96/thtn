(define (domain pick_place_coordination_rosplan)

(:requirements 
    :strips 
    :typing 
    :durative-actions 
    ;; :negative-preconditions 
    :timed-initial-literals
    :disjunctive-preconditions 
)

(:types
    item - object
    state - object
    robot - object
    location - object
    rail_block - object
)

(:predicates
    (actions-enabled)
    (safe_state ?s - state)
    (unsafe_state ?s - state)
    (gripper-free ?r - robot)
    (gripper-used ?r - robot)
    (occupied ?rb - rail_block)
    (notoccupied ?rb - rail_block)
    (grasped ?r - robot ?i - item)
    (notgrasped ?r - robot ?i - item)
    (item_at ?i - item ?l - location)
    (neighbors ?rb1 ?rb2 - rail_block)
    (inside ?r - robot ?rb - rail_block)
    (robot_config ?r - robot ?s - state)
    (robot_at ?r - robot ?rb - rail_block)
    (notinside ?r - robot ?rb - rail_block)
    (reachable ?rb - rail_block ?l - location)
)

(:durative-action move_to_home_state
    :parameters (?r - robot ?target ?current - state)
    :duration (= ?duration 10)
    :condition (and
        (over all (actions-enabled))
        (at start (safe_state ?target))
        (at start (unsafe_state ?current))
        (at start (robot_config ?r ?current))
    )
    :effect (and
        (at start (not (robot_config ?r ?current)))
        (at end (robot_config ?r ?target))
    )
)

(:durative-action rail_move
    :parameters (?r - robot ?home - state ?from ?to - rail_block)
    :duration (= ?duration 20)
    :condition (and
        (over all (actions-enabled))
        (at start (occupied ?from))
        (at start (notoccupied ?to))
        (at start (safe_state ?home))
        (at start (robot_at ?r ?from))
        (at start (robot_config ?r ?home))
        (over all (robot_config ?r ?home))
        (over all (neighbors ?from ?to))
    )
    :effect (and
        (at start (not (occupied ?from)))
        (at start (not (robot_at ?r ?from)))
        (at start (not (notoccupied ?to)))
        (at end (occupied ?to))
        (at end (robot_at ?r ?to))
        (at end (notoccupied ?from))
        (at end (robot_config ?r ?home))
    )
)

(:durative-action grasp
    :parameters (?r - robot ?i - item ?home ?target - state ?from - rail_block ?iloc - location)
    :duration (= ?duration 30)
    :condition (and
        (at start (gripper-free ?r))
        (at start (notgrasped ?r ?i))
        (over all (actions-enabled))
        (at start (item_at ?i ?iloc))
        (at start (safe_state ?home))
        (at start (robot_at ?r ?from))
        (over all (robot_at ?r ?from))
        (at start (unsafe_state ?target))
        (at start (reachable ?from ?iloc))
        (at start (robot_config ?r ?home))
    )
    :effect (and
        (at start (not (gripper-free ?r)))
        (at start (not (item_at ?i ?iloc)))
        ;; (at start (not (notgrasped ?r ?i)))
        (at start (not (robot_config ?r ?home)))
        (at end (grasped ?r ?i))
        (at end (gripper-used ?r))
        (at end (robot_at ?r ?from))
        (at end (robot_config ?r ?target))
    )
)

(:durative-action release
    :parameters (?r - robot ?i - item ?home ?target - state ?from - rail_block ?iloc - location)
    :duration (= ?duration 30)
    :condition (and
        (at start (grasped ?r ?i))
        (at start (gripper-used ?r))
        (over all (actions-enabled))
        (at start (safe_state ?home))
        (at start (robot_at ?r ?from))
        (over all (robot_at ?r ?from))
        (at start (unsafe_state ?target))
        (at start (reachable ?from ?iloc))
        (at start (robot_config ?r ?home))
    )
    :effect (and
        (at start (not (grasped ?r ?i)))
        (at start (not (gripper-used ?r)))
        (at start (not (robot_config ?r ?home)))
        (at end (gripper-free ?r))
        (at end (notgrasped ?r ?i))
        (at end (item_at ?i ?iloc))
        (at end (robot_at ?r ?from))
        (at end (robot_config ?r ?target))
    )

)

)
