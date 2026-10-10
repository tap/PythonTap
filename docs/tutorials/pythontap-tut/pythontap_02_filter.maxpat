{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 9,
			"minor": 1,
			"revision": 5,
			"architecture": "x64",
			"modernui": 1
		},
		"classnamespace": "box",
		"rect": [
			100.0,
			100.0,
			760.0,
			560.0
		],
		"bglocked": 0,
		"openinpresentation": 0,
		"default_fontsize": 13.0,
		"default_fontface": 0,
		"default_fontname": "Ableton Sans Light Regular",
		"gridonopen": 1,
		"gridsize": [
			5.0,
			5.0
		],
		"gridsnaponopen": 2,
		"objectsnaponopen": 0,
		"statusbarvisible": 2,
		"toolbarvisible": 1,
		"boxanimatetime": 200,
		"enablehscroll": 1,
		"enablevscroll": 1,
		"description": "",
		"digest": "",
		"tags": "",
		"style": "",
		"assistshowspatchername": 0,
		"textcolor": [
			0.847058823529412,
			0.847058823529412,
			0.847058823529412,
			1.0
		],
		"bgcolor": [
			0.109803921568627,
			0.109803921568627,
			0.109803921568627,
			1.0
		],
		"editing_bgcolor": [
			0.109803921568627,
			0.109803921568627,
			0.109803921568627,
			1.0
		],
		"boxes": [
			{
				"box": {
					"maxclass": "comment",
					"text": "PythonTap Tutorial 2",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						15.0,
						600.0,
						45.699999999999996
					],
					"fontsize": 26.0,
					"fontname": "Ableton Sans Bold Regular",
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "A filter that knows the sample rate",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						55.0,
						600.0,
						29.75
					],
					"fontsize": 15.0,
					"textcolor": [
						0.976470588235294,
						0.733333333333333,
						0.258823529411765,
						1.0
					],
					"id": "obj-2"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "The Documentation window has this tutorial's text. Turn on the audio with the speaker.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						90.0,
						330,
						45.699999999999996
					],
					"linecount": 2,
					"id": "obj-3"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "noise~",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						150.0,
						60.0,
						24.0
					],
					"id": "obj-4"
				}
			},
			{
				"box": {
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						185.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "delay",
					"id": "obj-5"
				}
			},
			{
				"box": {
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						180.0,
						185.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "alpha",
					"id": "obj-6"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "alpha 1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						180.0,
						220.0,
						77.19999999999999,
						24.0
					],
					"id": "obj-7"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "clear",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						265.2,
						220.0,
						62.0,
						24.0
					],
					"id": "obj-8"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python~ allpass",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						260.0,
						151.0,
						24.0
					],
					"id": "obj-9"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python~ numpy_allpass",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						220.0,
						260.0,
						193.0,
						24.0
					],
					"id": "obj-10"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						305.0,
						40.0,
						24.0
					],
					"id": "obj-11"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "2",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						68.0,
						305.0,
						40.0,
						24.0
					],
					"id": "obj-12"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "allpass.py, or numpy_allpass.py",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						120.0,
						305.0,
						220,
						45.699999999999996
					],
					"linecount": 2,
					"id": "obj-13"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "selector~ 2 1",
					"numinlets": 3,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						345.0,
						109.0,
						24.0
					],
					"id": "obj-14"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "+~",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						380.0,
						40.0,
						24.0
					],
					"id": "obj-15"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "with the dry signal: the notches",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						65.0,
						380.0,
						240,
						26.849999999999998
					],
					"id": "obj-16"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "*~ 0.1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						420.0,
						60.0,
						24.0
					],
					"id": "obj-17"
				}
			},
			{
				"box": {
					"maxclass": "ezdac~",
					"numinlets": 2,
					"numoutlets": 0,
					"outlettype": [],
					"patching_rect": [
						20.0,
						455.0,
						45.0,
						45.0
					],
					"id": "obj-18"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "metro 250 @active 1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						220.0,
						420.0,
						151.0,
						24.0
					],
					"id": "obj-19"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "adstatus cpu",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"",
						"int"
					],
					"patching_rect": [
						220.0,
						450.0,
						102.0,
						24.0
					],
					"id": "obj-20"
				}
			},
			{
				"box": {
					"maxclass": "number",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						"bang"
					],
					"patching_rect": [
						220.0,
						480.0,
						60.0,
						24.0
					],
					"parameter_enable": 0,
					"id": "obj-21"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "CPU %",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						285.0,
						480.0,
						60,
						26.849999999999998
					],
					"id": "obj-22"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Both objects get every message: the attrui and the message boxes reach the one you hear and the one you don't alike.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						420.0,
						150.0,
						300,
						64.55
					],
					"linecount": 3,
					"id": "obj-23"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "alpha 1 is refused by the class's validator: its traceback prints to the Max console, and alpha keeps its value.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						420.0,
						230.0,
						300,
						64.55
					],
					"linecount": 3,
					"id": "obj-24"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-4",
						0
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
						"obj-5",
						0
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
						"obj-6",
						0
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
						"obj-7",
						0
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
						"obj-8",
						0
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
						"obj-4",
						0
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
						"obj-5",
						0
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
						"obj-6",
						0
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
						"obj-7",
						0
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
						"obj-8",
						0
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
						"obj-11",
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
						"obj-12",
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
						"obj-9",
						0
					],
					"destination": [
						"obj-14",
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
						"obj-14",
						2
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
						"obj-4",
						0
					],
					"destination": [
						"obj-15",
						1
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
						"obj-17",
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
						"obj-18",
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
						"obj-18",
						1
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
						"obj-20",
						0
					],
					"destination": [
						"obj-21",
						0
					]
				}
			}
		]
	}
}
