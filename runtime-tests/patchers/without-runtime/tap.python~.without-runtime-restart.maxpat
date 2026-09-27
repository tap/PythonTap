{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 0,
			"revision": 8,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			60.0,
			80.0,
			1290,
			480.0
		],
		"default_fontsize": 12.0,
		"default_fontname": "Arial",
		"gridsize": [
			15.0,
			15.0
		],
		"description": "without-runtime/tap.python~.without-runtime-restart",
		"boxes": [
			{
				"box": {
					"id": "obj-1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						20.0,
						480,
						80.0
					],
					"text": "without-runtime/tap.python~.without-runtime-restart\n\nRun by run.py after putting support/ back while Max runs (plan 4.2, 6.4): on macOS the external was loaded without its runtime, and dyld never binds a weak import later, so a new object must say to restart Max \u2014 the documented limit, pinned here.",
					"linecount": 4
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						20.0,
						65.0,
						22.0
					],
					"text": "error 1"
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 5,
					"outlettype": [
						"",
						"",
						"",
						"",
						""
					],
					"patching_rect": [
						1020.0,
						50.0,
						177.0,
						22.0
					],
					"text": "regexp then.restart.Max"
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						80.0,
						40.0,
						22.0
					],
					"text": "t b"
				}
			},
			{
				"box": {
					"id": "obj-5",
					"maxclass": "newobj",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"",
						"",
						"",
						""
					],
					"patching_rect": [
						1020.0,
						110.0,
						135.0,
						22.0
					],
					"text": "counter 1 1000000"
				}
			},
			{
				"box": {
					"id": "obj-6",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						520.0,
						20.0,
						51.0,
						22.0
					],
					"text": "poly~"
				}
			},
			{
				"box": {
					"id": "obj-7",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						270.0,
						20.0,
						135.0,
						22.0
					],
					"text": "args maxtest_gain"
				}
			},
			{
				"box": {
					"id": "obj-8",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						270.0,
						50.0,
						72.0,
						22.0
					],
					"text": "voices 1"
				}
			},
			{
				"box": {
					"id": "obj-9",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						270.0,
						80.0,
						184.0,
						22.0
					],
					"text": "patchername maxtest.host"
				}
			},
			{
				"box": {
					"id": "obj-10",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						140.0,
						40.0,
						22.0
					],
					"text": "i"
				}
			},
			{
				"box": {
					"id": "obj-11",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						170.0,
						44.0,
						22.0
					],
					"text": "== 1"
				}
			},
			{
				"box": {
					"id": "obj-12",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						770.0,
						20.0,
						233.0,
						22.0
					],
					"text": "test.assert says-to-restart-max"
				}
			},
			{
				"box": {
					"id": "obj-13",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": [],
					"patching_rect": [
						20.0,
						108.0,
						114.0,
						22.0
					],
					"text": "test.terminate"
				}
			},
			{
				"box": {
					"id": "obj-14",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						200.0,
						72.0,
						22.0
					],
					"text": "tosymbol"
				}
			},
			{
				"box": {
					"id": "obj-15",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 5,
					"outlettype": [
						"",
						"",
						"",
						"",
						""
					],
					"patching_rect": [
						1020.0,
						230.0,
						177.0,
						22.0
					],
					"text": "regexp \\\" @substitute '"
				}
			},
			{
				"box": {
					"id": "obj-16",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						260.0,
						86.0,
						22.0
					],
					"text": "fromsymbol"
				}
			},
			{
				"box": {
					"id": "obj-17",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						1020.0,
						290.0,
						51.0,
						22.0
					],
					"text": "t l b"
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "newobj",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"",
						"",
						"",
						""
					],
					"patching_rect": [
						1020.0,
						320.0,
						135.0,
						22.0
					],
					"text": "counter 1 1000000"
				}
			},
			{
				"box": {
					"id": "obj-19",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						350.0,
						51.0,
						22.0
					],
					"text": "<= 50"
				}
			},
			{
				"box": {
					"id": "obj-20",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						1020.0,
						380.0,
						72.0,
						22.0
					],
					"text": "gate 1 1"
				}
			},
			{
				"box": {
					"id": "obj-21",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": [],
					"patching_rect": [
						1020.0,
						410.0,
						170.0,
						22.0
					],
					"text": "test.log console-error"
				}
			},
			{
				"box": {
					"id": "obj-22",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						138.0,
						72.0,
						22.0
					],
					"text": "loadbang"
				}
			},
			{
				"box": {
					"id": "obj-23",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						168.0,
						79.0,
						22.0
					],
					"text": "delay 150"
				}
			},
			{
				"box": {
					"id": "obj-24",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 4,
					"outlettype": [
						"",
						"",
						"",
						""
					],
					"patching_rect": [
						20.0,
						198.0,
						79.0,
						22.0
					],
					"text": "t b b b b"
				}
			},
			{
				"box": {
					"id": "obj-25",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						228.0,
						79.0,
						22.0
					],
					"text": "delay 150"
				}
			},
			{
				"box": {
					"id": "obj-26",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						20.0,
						258.0,
						51.0,
						22.0
					],
					"text": "t b b"
				}
			},
			{
				"box": {
					"id": "obj-27",
					"maxclass": "newobj",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						288.0,
						79.0,
						22.0
					],
					"text": "delay 500"
				}
			},
			{
				"box": {
					"id": "obj-28",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						20.0,
						318.0,
						51.0,
						22.0
					],
					"text": "t b b"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-2",
						0
					],
					"destination": [
						"obj-3",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-3",
						2
					],
					"destination": [
						"obj-4",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-4",
						0
					],
					"destination": [
						"obj-5",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-7",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-8",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-9",
						0
					],
					"destination": [
						"obj-6",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-5",
						0
					],
					"destination": [
						"obj-10",
						1
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-10",
						0
					],
					"destination": [
						"obj-11",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-11",
						0
					],
					"destination": [
						"obj-12",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-2",
						0
					],
					"destination": [
						"obj-14",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-14",
						0
					],
					"destination": [
						"obj-15",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-15",
						0
					],
					"destination": [
						"obj-16",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-15",
						3
					],
					"destination": [
						"obj-16",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-16",
						0
					],
					"destination": [
						"obj-17",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-17",
						1
					],
					"destination": [
						"obj-18",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-18",
						0
					],
					"destination": [
						"obj-19",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-19",
						0
					],
					"destination": [
						"obj-20",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-17",
						0
					],
					"destination": [
						"obj-20",
						1
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-20",
						0
					],
					"destination": [
						"obj-21",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-22",
						0
					],
					"destination": [
						"obj-23",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-23",
						0
					],
					"destination": [
						"obj-24",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-24",
						3
					],
					"destination": [
						"obj-7",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-24",
						2
					],
					"destination": [
						"obj-8",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-24",
						1
					],
					"destination": [
						"obj-9",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-24",
						0
					],
					"destination": [
						"obj-25",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-25",
						0
					],
					"destination": [
						"obj-26",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-26",
						1
					],
					"destination": [
						"obj-10",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-26",
						0
					],
					"destination": [
						"obj-27",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-27",
						0
					],
					"destination": [
						"obj-28",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-28",
						1
					],
					"destination": [
						"obj-13",
						0
					]
				}
			}
		]
	}
}
