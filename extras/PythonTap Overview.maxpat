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
			620.0
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
					"text": "PythonTap",
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
					"text": "Write Max objects in Python",
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
					"text": "PythonTap runs a Python class as a Max object: its typed fields become attributes, its methods messages, and saving the file reloads it in place. The classes live in the package's python folder; CPython 3.13, numpy and attrs come with it.",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						90.0,
						360,
						102.25
					],
					"linecount": 5,
					"id": "obj-3"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "tap.python~ — process audio with a Python class",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						200.0,
						360,
						26.849999999999998
					],
					"fontname": "Ableton Sans Bold Regular",
					"id": "obj-4"
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
						235.0,
						74.0,
						24.0
					],
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
						110.0,
						235.0,
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
					"maxclass": "newobj",
					"text": "tap.python~ numpy_gain",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"signal"
					],
					"patching_rect": [
						20.0,
						270.0,
						172.0,
						24.0
					],
					"id": "obj-7"
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
						305.0,
						60.0,
						24.0
					],
					"id": "obj-8"
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
						340.0,
						45.0,
						45.0
					],
					"id": "obj-9"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "help tap.python~",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						200.0,
						305.0,
						145.6,
						24.0
					],
					"id": "obj-10"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "tap.python — a Python class as a Max object: what a method returns, it outputs",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						400.0,
						360,
						45.699999999999996
					],
					"linecount": 2,
					"fontname": "Ableton Sans Bold Regular",
					"id": "obj-11"
				}
			},
			{
				"box": {
					"maxclass": "button",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						20.0,
						455.0,
						24.0,
						24.0
					],
					"parameter_enable": 0,
					"id": "obj-12"
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
						55.0,
						455.0,
						150,
						24.0
					],
					"parameter_enable": 0,
					"attr": "steps",
					"id": "obj-13"
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
						20.0,
						490.0,
						137.0,
						24.0
					],
					"id": "obj-14"
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
						20.0,
						525.0,
						95.0,
						24.0
					],
					"id": "obj-15"
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
						20.0,
						560.0,
						200,
						24.0
					],
					"id": "obj-16"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "help tap.python",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						220.0,
						490.0,
						138.0,
						24.0
					],
					"id": "obj-17"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Learn",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						440.0,
						200.0,
						300,
						26.849999999999998
					],
					"fontname": "Ableton Sans Bold Regular",
					"id": "obj-18"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "The guide, Writing Max Objects in Python, and three tutorials are in the Documentation window: Reference > Package Docs > PythonTap. The tutorials' patchers:",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						440.0,
						230.0,
						330,
						83.39999999999999
					],
					"linecount": 4,
					"id": "obj-19"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load pythontap_01_gain.maxpat",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						305.0,
						244.39999999999998,
						24.0
					],
					"id": "obj-20"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load pythontap_02_filter.maxpat",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						340.0,
						259.6,
						24.0
					],
					"id": "obj-21"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": "load pythontap_03_control.maxpat",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						375.0,
						267.2,
						24.0
					],
					"id": "obj-22"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "pcontrol",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						555.0,
						74.0,
						24.0
					],
					"id": "obj-23"
				}
			},
			{
				"box": {
					"maxclass": "comment",
					"text": "Every rule, the performance measurements, and how to add Python packages to the runtime:",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						440.0,
						420.0,
						330,
						45.699999999999996
					],
					"linecount": 2,
					"id": "obj-24"
				}
			},
			{
				"box": {
					"maxclass": "message",
					"text": ";\rmax launchbrowser https://github.com/tap/PythonTap#readme",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						440.0,
						465.0,
						330,
						42.0
					],
					"linecount": 2,
					"id": "obj-25"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"source": [
						"obj-5",
						0
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
						"obj-6",
						0
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
						"obj-7",
						0
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
						"obj-8",
						0
					],
					"destination": [
						"obj-9",
						1
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
						"obj-13",
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
						"obj-20",
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
						"obj-21",
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
						"obj-10",
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
						"obj-17",
						0
					],
					"destination": [
						"obj-23",
						0
					]
				}
			}
		]
	}
}
