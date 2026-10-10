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
			800.0,
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
					"maxclass": "newobj",
					"text": "p basic",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						20.0,
						67.0,
						24.0
					],
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
							900.0,
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
									"text": "tap.python",
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
									"text": "Run a Python class as a Max object",
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
									"text": "[tap.python name] loads python/name.py from this package and makes an object of the class name in it: its typed fields are attributes, its methods are messages, and what a method returns is what the object outputs.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										90.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-3"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "This one is euclid.py. A bang outputs a Euclidean rhythm: steps beats, pulses of them on, spread as evenly as they go. steps and pulses are attributes, from the class's two typed fields.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										185.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-4"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "Open python/euclid.py in a text editor to see the class. Save a change and the object loads it again, keeping steps and pulses; a mistake prints its traceback to the Max console, and the object outputs nothing until a save fixes it.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										280.0,
										330.0,
										121.1
									],
									"linecount": 6,
									"id": "obj-5"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "Every object using the file shares one load of it per save, tap.python~ objects included.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										390.0,
										330.0,
										45.699999999999996
									],
									"linecount": 2,
									"id": "obj-6"
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
										400.0,
										90.0,
										24.0,
										24.0
									],
									"parameter_enable": 0,
									"id": "obj-7"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "bang: output the rhythm",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										430.0,
										90.0,
										200,
										26.849999999999998
									],
									"id": "obj-8"
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
										400.0,
										130.0,
										150,
										24.0
									],
									"parameter_enable": 0,
									"attr": "steps",
									"id": "obj-9"
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
										560.0,
										130.0,
										150,
										24.0
									],
									"parameter_enable": 0,
									"attr": "pulses",
									"id": "obj-10"
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
										560.0,
										170.0,
										84.8,
										24.0
									],
									"id": "obj-11"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "filechanged",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										652.8,
										170.0,
										107.6,
										24.0
									],
									"id": "obj-12"
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
										400.0,
										215.0,
										137.0,
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
										400.0,
										265.0,
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
										400.0,
										300.0,
										230,
										24.0
									],
									"id": "obj-15"
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
										650.0,
										265.0,
										95.0,
										24.0
									],
									"id": "obj-16"
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
										650.0,
										300.0,
										120,
										24.0
									],
									"id": "obj-17"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "what a method returns",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										335.0,
										200,
										26.849999999999998
									],
									"id": "obj-18"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "the dumpout: get + an attribute's name",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										650.0,
										335.0,
										190,
										45.699999999999996
									],
									"linecount": 2,
									"id": "obj-19"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "filechanged reloads the file at once; saving it does that for you.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										390.0,
										330,
										45.699999999999996
									],
									"linecount": 2,
									"id": "obj-20"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "More in the other tabs: lists, several outlets, messages and methods, threads.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										435.0,
										330,
										45.699999999999996
									],
									"linecount": 2,
									"id": "obj-21"
								}
							}
						],
						"lines": [
							{
								"patchline": {
									"source": [
										"obj-7",
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
										"obj-9",
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
										"obj-11",
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
										"obj-13",
										1
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
							}
						],
						"showontab": 1
					},
					"id": "obj-1"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "p lists",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						120.0,
						20.0,
						67.0,
						24.0
					],
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
							900.0,
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
									"text": "Lists in, lists out",
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
									"text": "A list message through numpy",
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
									"text": "scale.py's method list(self, values: np.ndarray) takes the whole list as one numpy array — a last parameter hinted np.ndarray, or list[float], takes every argument left — and returns an array, which the object outputs as a list.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										90.0,
										330.0,
										121.1
									],
									"linecount": 6,
									"id": "obj-3"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "What a method returns is output by its type: a number as a number, a str as symbol and the string, and a list, a range or a numpy array as a list.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										205.0,
										330.0,
										83.39999999999999
									],
									"linecount": 4,
									"id": "obj-4"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "A list whose first element is a str outputs the message it names: return [\"note\", 60, 100] and the object outputs note 60 100. A dict, a set or an object outputs nothing, and the console says why.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										285.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-5"
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
										400.0,
										90.0,
										77.19999999999999,
										24.0
									],
									"id": "obj-6"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "0.5 0.25 0.125",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										485.2,
										90.0,
										130.39999999999998,
										24.0
									],
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
										400.0,
										130.0,
										150,
										24.0
									],
									"parameter_enable": 0,
									"attr": "factor",
									"id": "obj-8"
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
										560.0,
										130.0,
										150,
										24.0
									],
									"parameter_enable": 0,
									"attr": "offset",
									"id": "obj-9"
								}
							},
							{
								"box": {
									"maxclass": "newobj",
									"text": "tap.python scale @factor 2 @offset 1",
									"numinlets": 1,
									"numoutlets": 2,
									"outlettype": [
										"",
										""
									],
									"patching_rect": [
										400.0,
										175.0,
										270.0,
										24.0
									],
									"id": "obj-10"
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
										400.0,
										225.0,
										95.0,
										24.0
									],
									"id": "obj-11"
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
										400.0,
										260.0,
										300,
										24.0
									],
									"id": "obj-12"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "each value times factor, plus offset",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										295.0,
										300,
										26.849999999999998
									],
									"id": "obj-13"
								}
							}
						],
						"lines": [
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
							}
						],
						"showontab": 1
					},
					"id": "obj-2"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "p outlets",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						220.0,
						20.0,
						81.0,
						24.0
					],
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
							900.0,
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
									"text": "One value per outlet",
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
									"text": "A method hinted to return a tuple",
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
									"text": "note_name.py's method int(self, note: int) -> tuple[str, int] returns two values, so the object has two outlets for them, and outputs them right to left, as Max objects do: the octave, then the name.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										90.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-3"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "The name is a str, which the object outputs as symbol and the string: sel matches it, and route needs route symbol first. Return it in a list, [\"C\"], to output the message C instead.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										205.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-4"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "The object has as many outlets as the widest tuple return hint among the class's methods — a method returning one value fills the first — plus the dumpout at the right. A save that changes the number changes the outlets in place, keeping their patch cords.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										305.0,
										330.0,
										121.1
									],
									"linecount": 6,
									"id": "obj-5"
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
										400.0,
										90.0,
										168.0,
										53.0
									],
									"parameter_enable": 0,
									"id": "obj-6"
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
										615.0,
										105.0,
										60.0,
										24.0
									],
									"parameter_enable": 0,
									"id": "obj-7"
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
										400.0,
										175.0,
										158.0,
										24.0
									],
									"id": "obj-8"
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
										400.0,
										225.0,
										69.6,
										24.0
									],
									"id": "obj-9"
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
										400.0,
										260.0,
										120,
										24.0
									],
									"id": "obj-10"
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
										540.0,
										225.0,
										60.0,
										24.0
									],
									"parameter_enable": 0,
									"id": "obj-11"
								}
							},
							{
								"box": {
									"maxclass": "newobj",
									"text": "sel C",
									"numinlets": 2,
									"numoutlets": 2,
									"outlettype": [
										"",
										""
									],
									"patching_rect": [
										620.0,
										225.0,
										53.0,
										24.0
									],
									"id": "obj-12"
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
										620.0,
										260.0,
										24.0,
										24.0
									],
									"parameter_enable": 0,
									"id": "obj-13"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "name",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										290.0,
										120,
										26.849999999999998
									],
									"id": "obj-14"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "octave",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										540.0,
										255.0,
										70,
										26.849999999999998
									],
									"id": "obj-15"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "every C",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										650.0,
										260.0,
										80,
										26.849999999999998
									],
									"id": "obj-16"
								}
							}
						],
						"lines": [
							{
								"patchline": {
									"source": [
										"obj-6",
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
										"obj-8",
										1
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
										"obj-8",
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
										"obj-12",
										0
									],
									"destination": [
										"obj-13",
										0
									]
								}
							}
						],
						"showontab": 1
					},
					"id": "obj-3"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "p messages",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						320.0,
						20.0,
						88.0,
						24.0
					],
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
							900.0,
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
									"text": "Messages and methods",
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
									"text": "The class's methods are the object's messages",
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
									"text": "With no argument the object loads python/default.py, which tap.python~ loads too. Its public methods are messages, called according to their signatures: bang returns the gain, float and int set it, greet prints to the Max console.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										90.0,
										330.0,
										121.1
									],
									"linecount": 6,
									"id": "obj-3"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "process() is an ordinary method here: process 0.25 calls it, and outputs what it returns — a way to try a tap.python~ class sample by sample.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										205.0,
										330.0,
										83.39999999999999
									],
									"linecount": 4,
									"id": "obj-4"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "A method named anything answers every message the class has no method or attribute for, with the message's name first: def anything(self, selector: str, *args). Without one, the object doesn't understand it — try nonesuch.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										280.0,
										330.0,
										102.25
									],
									"linecount": 5,
									"id": "obj-5"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "Names Max and the object keep — dumpout, filechanged, assist and Max's own messages — are not exposed: the console says so, so that you can rename the method.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										380.0,
										330.0,
										83.39999999999999
									],
									"linecount": 4,
									"id": "obj-6"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "bang",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										400.0,
										90.0,
										54.4,
										24.0
									],
									"id": "obj-7"
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
										462.4,
										90.0,
										92.39999999999999,
										24.0
									],
									"id": "obj-8"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "int 2",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										562.8,
										90.0,
										62.0,
										24.0
									],
									"id": "obj-9"
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
										632.8,
										90.0,
										92.39999999999999,
										24.0
									],
									"id": "obj-10"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "process 0.25",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										400.0,
										125.0,
										115.19999999999999,
										24.0
									],
									"id": "obj-11"
								}
							},
							{
								"box": {
									"maxclass": "message",
									"text": "nonesuch 1",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										523.2,
										125.0,
										100.0,
										24.0
									],
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
										400.0,
										165.0,
										150,
										24.0
									],
									"parameter_enable": 0,
									"attr": "gain",
									"id": "obj-13"
								}
							},
							{
								"box": {
									"maxclass": "newobj",
									"text": "tap.python",
									"numinlets": 1,
									"numoutlets": 2,
									"outlettype": [
										"",
										""
									],
									"patching_rect": [
										400.0,
										210.0,
										88.0,
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
										400.0,
										260.0,
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
										400.0,
										295.0,
										120,
										24.0
									],
									"id": "obj-16"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "what the method returned",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										530.0,
										295.0,
										180,
										26.849999999999998
									],
									"id": "obj-17"
								}
							}
						],
						"lines": [
							{
								"patchline": {
									"source": [
										"obj-7",
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
										"obj-8",
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
										"obj-14",
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
							}
						],
						"showontab": 1
					},
					"id": "obj-4"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "p threads",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						420.0,
						20.0,
						81.0,
						24.0
					],
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
							900.0,
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
									"text": "Threads",
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
									"text": "A message runs on the thread it arrives on",
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
									"text": "As with any Max object, a message runs on the thread it arrives on, and what the method returns is output before the message returns. From a click or a message box that is Max's main thread; from a metro with Overdrive on, the scheduler thread — where a long method holds the scheduler while it runs.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										90.0,
										330.0,
										139.95
									],
									"linecount": 7,
									"id": "obj-3"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "With Scheduler in Audio Interrupt on, and audio running, the scheduler thread is the audio thread: Python then runs on it, where a long method, a reload or another object's Python can interrupt the audio. The object says so in the console, once. deferlow moves the message to the main thread.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										225.0,
										330.0,
										139.95
									],
									"linecount": 7,
									"id": "obj-4"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "Output is what a method returns: a thread the class starts itself has no outlet to reach.",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										20.0,
										360.0,
										330.0,
										45.699999999999996
									],
									"linecount": 2,
									"id": "obj-5"
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
										400.0,
										90.0,
										24.0,
										24.0
									],
									"parameter_enable": 0,
									"id": "obj-6"
								}
							},
							{
								"box": {
									"maxclass": "newobj",
									"text": "metro 125",
									"numinlets": 2,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										400.0,
										125.0,
										81.0,
										24.0
									],
									"id": "obj-7"
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
										400.0,
										210.0,
										137.0,
										24.0
									],
									"id": "obj-8"
								}
							},
							{
								"box": {
									"maxclass": "newobj",
									"text": "deferlow",
									"numinlets": 1,
									"numoutlets": 1,
									"outlettype": [
										""
									],
									"patching_rect": [
										550.0,
										165.0,
										74.0,
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
										550.0,
										210.0,
										137.0,
										24.0
									],
									"id": "obj-10"
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
										400.0,
										260.0,
										95.0,
										24.0
									],
									"id": "obj-11"
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
										400.0,
										295.0,
										140,
										24.0
									],
									"id": "obj-12"
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
										550.0,
										260.0,
										95.0,
										24.0
									],
									"id": "obj-13"
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
										550.0,
										295.0,
										140,
										24.0
									],
									"id": "obj-14"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "on the scheduler thread (Overdrive on)",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										400.0,
										335.0,
										140,
										64.55
									],
									"linecount": 3,
									"id": "obj-15"
								}
							},
							{
								"box": {
									"maxclass": "comment",
									"text": "on the main thread",
									"numinlets": 1,
									"numoutlets": 0,
									"patching_rect": [
										550.0,
										335.0,
										140,
										26.849999999999998
									],
									"id": "obj-16"
								}
							}
						],
						"lines": [
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
										"obj-8",
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
										"obj-10",
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
										"obj-13",
										0
									],
									"destination": [
										"obj-14",
										0
									]
								}
							}
						],
						"showontab": 1
					},
					"id": "obj-5"
				}
			},
			{
				"box": {
					"maxclass": "newobj",
					"text": "p ?",
					"numinlets": 0,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						20.0,
						40.0,
						24.0
					],
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
							900.0,
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
						"boxes": [],
						"lines": [],
						"showontab": 1
					},
					"id": "obj-6"
				}
			}
		],
		"lines": [],
		"showrootpatcherontab": 0,
		"showontab": 0
	}
}
