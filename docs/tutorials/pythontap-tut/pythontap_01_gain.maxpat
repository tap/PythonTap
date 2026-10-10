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
					"text": "PythonTap Tutorial 1",
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
					"text": "A gain, per sample and per vector",
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
					"text": "saw~ 110",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						150.0,
						74.0,
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
						190.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "gain",
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
						200.0,
						190.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "gain",
					"id": "obj-6"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "float 0.5",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						225.0,
						92.39999999999999,
						24.0
					],
					"id": "obj-7"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "greet max",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						120.39999999999999,
						225.0,
						92.39999999999999,
						24.0
					],
					"id": "obj-8"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python~ default",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						265.0,
						151.0,
						24.0
					],
					"id": "obj-9"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "tap.python~ numpy_gain",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						200.0,
						265.0,
						172.0,
						24.0
					],
					"id": "obj-10"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "per sample: default.py",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						295.0,
						170,
						26.849999999999998
					],
					"id": "obj-11"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "per vector: numpy_gain.py",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						200.0,
						295.0,
						190,
						26.849999999999998
					],
					"id": "obj-12"
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
						335.0,
						40.0,
						24.0
					],
					"id": "obj-13"
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
						335.0,
						40.0,
						24.0
					],
					"id": "obj-14"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "listen to one or the other",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						120.0,
						335.0,
						220,
						26.849999999999998
					],
					"id": "obj-15"
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
						375.0,
						109.0,
						24.0
					],
					"id": "obj-16"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "*~ 0.2",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						415.0,
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
						450.0,
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
						200.0,
						415.0,
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
						200.0,
						445.0,
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
						200.0,
						475.0,
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
						265.0,
						475.0,
						60,
						26.849999999999998
					],
					"id": "obj-22"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Make your own: save a copy of default.py as python/mygain.py, rename its class mygain, and type tap.python~ mygain into a new object box. Every save reloads it.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						420.0,
						150.0,
						300,
						83.39999999999999
					],
					"linecount": 4,
					"id": "obj-23"
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
						"obj-13",
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
						"obj-14",
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
						"obj-9",
						0
					],
					"destination": [
						"obj-16",
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
						"obj-16",
						2
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
