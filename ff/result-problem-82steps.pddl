(define (problem bots)

	(:domain
		bots-domain
	)

	(:objects
		bot0 bot1 - bot

		area0 area1 area2 area3 area4 area5 area6 area7 area8 area9 area10 area11 area12 area13 - waypoint
		door0 door1 door2 door3 door4 door5 door6 door7 door8 door9 - door

		button0 button1 button2 button3 button4 button5 button6 button7 button8 button9 - button
		weapon0 weapon1 - weapon
		box0 box1 - box
	)

	(:init
		(sniper-weapon weapon0)
		(weapon-at weapon0 area3)

		(physcannon weapon1)
		(weapon-at weapon1 area7)

		(at bot0 area1) (empty bot0)

		(at bot1 area2) (empty bot1)

		(box-at box0 area1)
		(box-at box1 area12)

		(button-at button0 area8)
		(button-at button1 area7)
		(button-at button2 area6)
		(button-at button3 area5)
		(button-at button4 area4)
		(button-at button5 area3)
		(button-at button6 area13)
		(button-at button7 area2)
		(button-at button8 area9)
		(can-shoot button9 area9)

		(between door0 area8 area2)
		(between door1 area2 area7)
		(between door2 area6 area2)
		(between door3 area5 area6)
		(between door4 area5 area1)
		(between door5 area4 area1)
		(between door6 area1 area3)
		(between door7 area9 area5)
		(between door8 area12 area13)
		(between door9 area10 area9)

		(wall area10 area11)
		(wall area12 area11)

		; Auto-generated buttons configuration.
        (toggle button0 door0 door5)
        (toggle button1 door3 door1)
        (toggle button2 door6 door6)
        (toggle button3 door7 door2)
        (toggle button4 door8 door0)
        (toggle button5 door9 door6)
        (toggle button6 door7 door0)
        (toggle button7 door1 door4)
        (toggle button8 door5 door7)
        (toggle button9 door0 door9)
	)

	(:goal
		(and
			(at bot0 area13)
			(at bot1 area13)
		)
	)
)
