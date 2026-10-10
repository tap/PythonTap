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
			820.0,
			700.0
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
					"text": "PythonTap Tutorial 3",
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
					"text": "An object without audio",
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
					"text": "The Documentation window has this tutorial's text. Turn on the metro, and the audio to hear the rhythm.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						90.0,
						360,
						64.55
					],
					"linecount": 3,
					"id": "obj-3"
				}
			},
			{
				"box": {
					"maxclass": "toggle",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"int"
					],
					"patching_rect": [
						20.0,
						150.0,
						24.0,
						24.0
					],
					"parameter_enable": 0,
					"id": "obj-4"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "metro 150",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						185.0,
						81.0,
						24.0
					],
					"id": "obj-5"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "t b b",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"bang",
						"bang"
					],
					"patching_rect": [
						20.0,
						220.0,
						53.0,
						24.0
					],
					"id": "obj-6"
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
						120.0,
						220.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "steps",
					"id": "obj-7"
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
						280.0,
						220.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "pulses",
					"id": "obj-8"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "getsteps",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						220.0,
						84.8,
						24.0
					],
					"id": "obj-9"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python euclid",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						120.0,
						260.0,
						137.0,
						24.0
					],
					"id": "obj-10"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "t l l",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						120.0,
						295.0,
						53.0,
						24.0
					],
					"id": "obj-11"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "prepend set",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						280.0,
						330.0,
						95.0,
						24.0
					],
					"id": "obj-12"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						280.0,
						365.0,
						200,
						24.0
					],
					"id": "obj-13"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "prepend set",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						500.0,
						295.0,
						95.0,
						24.0
					],
					"id": "obj-14"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						500.0,
						330.0,
						110,
						24.0
					],
					"id": "obj-15"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "the dumpout",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						500.0,
						365.0,
						100,
						26.849999999999998
					],
					"id": "obj-16"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "zl.len",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						120.0,
						330.0,
						60.0,
						24.0
					],
					"id": "obj-17"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "counter",
					"numinlets": 3,
					"numoutlets": 4,
					"outlettype": [
						"int",
						"",
						"",
						"int"
					],
					"patching_rect": [
						20.0,
						330.0,
						67.0,
						24.0
					],
					"id": "obj-18"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "%",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"int"
					],
					"patching_rect": [
						20.0,
						365.0,
						40.0,
						24.0
					],
					"id": "obj-19"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "zl.lookup",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						20.0,
						400.0,
						81.0,
						24.0
					],
					"id": "obj-20"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "sel 1",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"bang",
						""
					],
					"patching_rect": [
						20.0,
						435.0,
						53.0,
						24.0
					],
					"id": "obj-21"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "click~",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						470.0,
						60.0,
						24.0
					],
					"id": "obj-22"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "*~ 0.5",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						505.0,
						60.0,
						24.0
					],
					"id": "obj-23"
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
						540.0,
						45.0,
						45.0
					],
					"id": "obj-24"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "note_name.py: two outlets, right to left",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						280.0,
						410.0,
						260,
						45.699999999999996
					],
					"linecount": 2,
					"id": "obj-25"
				}
			},
			{
				"box": {
					"maxclass": "kslider",
					"numinlets": 2,
					"numoutlets": 2,
					"outlettype": [
						"int",
						"int"
					],
					"patching_rect": [
						280.0,
						440.0,
						168.0,
						53.0
					],
					"parameter_enable": 0,
					"id": "obj-26"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python note_name",
					"numinlets": 1,
					"numoutlets": 3,
					"outlettype": [
						"",
						"",
						""
					],
					"patching_rect": [
						280.0,
						500.0,
						158.0,
						24.0
					],
					"id": "obj-27"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "set $1",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						280.0,
						535.0,
						69.6,
						24.0
					],
					"id": "obj-28"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						280.0,
						570.0,
						60,
						24.0
					],
					"id": "obj-29"
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
						360.0,
						570.0,
						60.0,
						24.0
					],
					"parameter_enable": 0,
					"id": "obj-30"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "scale.py: a list through numpy",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						405.0,
						230,
						26.849999999999998
					],
					"id": "obj-31"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "1 2 3 4",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						435.0,
						77.19999999999999,
						24.0
					],
					"id": "obj-32"
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
						520.0,
						470.0,
						140,
						24.0
					],
					"parameter_enable": 0,
					"attr": "factor",
					"id": "obj-33"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python scale",
					"numinlets": 1,
					"numoutlets": 2,
					"outlettype": [
						"",
						""
					],
					"patching_rect": [
						520.0,
						500.0,
						130.0,
						24.0
					],
					"id": "obj-34"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "prepend set",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						535.0,
						95.0,
						24.0
					],
					"id": "obj-35"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						570.0,
						150,
						24.0
					],
					"id": "obj-36"
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
						"obj-5",
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
						"obj-6",
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
						"obj-9",
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
						1
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
						"obj-12",
						0
					],
					"destination": [
						"obj-13",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-10",
						1
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
						"obj-11",
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
						"obj-6",
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
						"obj-17",
						0
					],
					"destination": [
						"obj-19",
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
						"obj-11",
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
						"obj-21",
						0
					],
					"destination": [
						"obj-22",
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
						"obj-23",
						0
					],
					"destination": [
						"obj-24",
						1
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
						0
					],
					"destination": [
						"obj-29",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-27",
						1
					],
					"destination": [
						"obj-30",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-32",
						0
					],
					"destination": [
						"obj-34",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-33",
						0
					],
					"destination": [
						"obj-34",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-34",
						0
					],
					"destination": [
						"obj-35",
						0
					]
				}
			},
			{
				"patchline": {
					"source": [
						"obj-35",
						0
					],
					"destination": [
						"obj-36",
						0
					]
				}
			}
		]
	}
}
