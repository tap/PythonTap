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
        "rect": [ 100.0, 100.0, 800.0, 620.0 ],
        "default_fontsize": 13.0,
        "default_fontname": "Ableton Sans Light Regular",
        "gridsize": [ 5.0, 5.0 ],
        "gridsnaponopen": 2,
        "objectsnaponopen": 0,
        "showrootpatcherontab": 0,
        "showontab": 0,
        "boxes": [
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 100.0, 152.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontsize": 26.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 15.0, 600.0, 38.0 ],
                                    "text": "tap.python"
                                }
                            },
                            {
                                "box": {
                                    "fontsize": 15.0,
                                    "id": "obj-2",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 55.0, 600.0, 24.0 ],
                                    "text": "Run a Python class as a Max object",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 316.0, 85.0 ],
                                    "text": "[tap.python name] loads python/name.py from this package and makes an object of the class name in it: its typed fields are attributes, its methods are messages, and what a method returns is what the object outputs."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 185.0, 278.0, 85.0 ],
                                    "text": "This one is euclid.py. A bang outputs a Euclidean rhythm: steps beats, pulses of them on, spread as evenly as they go. steps and pulses are attributes, from the class's two typed fields."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 280.0, 264.0, 100.0 ],
                                    "text": "Open python/euclid.py in a text editor to see the class. Save a change and the object loads it again, keeping steps and pulses; a mistake prints its traceback to the Max console, and the object outputs nothing until a save fixes it."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 390.0, 330.0, 38.0 ],
                                    "text": "Every object using the file shares one load of it per save, tap.python~ objects included."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "button",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 90.0, 24.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 430.0, 90.0, 200.0, 22.0 ],
                                    "text": "bang: output the rhythm"
                                }
                            },
                            {
                                "box": {
                                    "attr": "steps",
                                    "id": "obj-9",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 130.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "pulses",
                                    "id": "obj-10",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 560.0, 130.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 560.0, 170.0, 84.8, 24.0 ],
                                    "text": "getsteps"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 652.8, 170.0, 107.6, 24.0 ],
                                    "text": "filechanged"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "" ],
                                    "patching_rect": [ 400.0, 215.0, 137.0, 24.0 ],
                                    "text": "tap.python euclid"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 265.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 300.0, 230.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 650.0, 265.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-17",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 650.0, 300.0, 120.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-18",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 335.0, 200.0, 22.0 ],
                                    "text": "what a method returns"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-19",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 650.0, 335.0, 190.0, 38.0 ],
                                    "text": "the dumpout: get + an attribute's name"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-20",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 390.0, 330.0, 38.0 ],
                                    "text": "filechanged reloads the file at once; saving it does that for you."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-21",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 435.0, 330.0, 38.0 ],
                                    "text": "More in the other tabs: lists, several outlets, messages and methods, threads."
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-13", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-16", 0 ],
                                    "source": [ "obj-13", 1 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-14", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-17", 0 ],
                                    "source": [ "obj-16", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 20.0, 20.0, 67.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p basic"
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 0.0, 26.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontsize": 26.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 15.0, 600.0, 38.0 ],
                                    "text": "Lists in, lists out"
                                }
                            },
                            {
                                "box": {
                                    "fontsize": 15.0,
                                    "id": "obj-2",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 55.0, 600.0, 24.0 ],
                                    "text": "A list message through numpy",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 266.0, 100.0 ],
                                    "text": "scale.py's method list(self, values: np.ndarray) takes the whole list as one numpy array — a last parameter hinted np.ndarray, or list[float], takes every argument left — and returns an array, which the object outputs as a list."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 205.0, 288.0, 69.0 ],
                                    "text": "What a method returns is output by its type: a number as a number, a str as symbol and the string, and a list, a range or a numpy array as a list."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 285.0, 288.0, 85.0 ],
                                    "text": "A list whose first element is a str outputs the message it names: return [\"note\", 60, 100] and the object outputs note 60 100. A dict, a set or an object outputs nothing, and the console says why."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 90.0, 77.19999999999999, 24.0 ],
                                    "text": "1 2 3 4"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 485.2, 90.0, 130.39999999999998, 24.0 ],
                                    "text": "0.5 0.25 0.125"
                                }
                            },
                            {
                                "box": {
                                    "attr": "factor",
                                    "id": "obj-8",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 130.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "offset",
                                    "id": "obj-9",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 560.0, 130.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "" ],
                                    "patching_rect": [ 400.0, 175.0, 270.0, 24.0 ],
                                    "text": "tap.python scale @factor 2 @offset 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 225.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 260.0, 300.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 295.0, 300.0, 22.0 ],
                                    "text": "each value times factor, plus offset"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-11", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-6", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 120.0, 20.0, 67.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p lists"
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 0.0, 26.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontsize": 26.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 15.0, 600.0, 38.0 ],
                                    "text": "One value per outlet"
                                }
                            },
                            {
                                "box": {
                                    "fontsize": 15.0,
                                    "id": "obj-2",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 55.0, 600.0, 24.0 ],
                                    "text": "A method hinted to return a tuple",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 288.0, 85.0 ],
                                    "text": "note_name.py's method int(self, note: int) -> tuple[str, int] returns two values, so the object has two outlets for them, and outputs them right to left, as Max objects do: the octave, then the name."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 205.0, 252.0, 85.0 ],
                                    "text": "The name is a str, which the object outputs as symbol and the string: sel matches it, and route needs route symbol first. Return it in a list, [\"C\"], to output the message C instead."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 305.0, 304.0, 100.0 ],
                                    "text": "The object has as many outlets as the widest tuple return hint among the class's methods — a method returning one value fills the first — plus the dumpout at the right. A save that changes the number changes the outlets in place, keeping their patch cords."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "kslider",
                                    "numinlets": 2,
                                    "numoutlets": 2,
                                    "outlettype": [ "int", "int" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 90.0, 196.0, 34.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "number",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 615.0, 105.0, 60.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 3,
                                    "outlettype": [ "", "", "" ],
                                    "patching_rect": [ 400.0, 175.0, 158.0, 24.0 ],
                                    "text": "tap.python note_name"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 225.0, 69.6, 24.0 ],
                                    "text": "set $1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 260.0, 120.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "number",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 540.0, 225.0, 60.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 2,
                                    "outlettype": [ "bang", "" ],
                                    "patching_rect": [ 620.0, 225.0, 53.0, 24.0 ],
                                    "text": "sel C"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "button",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 620.0, 260.0, 24.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 290.0, 120.0, 22.0 ],
                                    "text": "name"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 540.0, 255.0, 70.0, 22.0 ],
                                    "text": "octave"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 650.0, 260.0, 80.0, 22.0 ],
                                    "text": "every C"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-8", 0 ],
                                    "source": [ "obj-6", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-8", 0 ],
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-11", 0 ],
                                    "source": [ "obj-8", 1 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
                                    "order": 0,
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-9", 0 ],
                                    "order": 1,
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 220.0, 20.0, 81.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p outlets"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 0.0, 26.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontsize": 26.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 15.0, 600.0, 38.0 ],
                                    "text": "Messages and methods"
                                }
                            },
                            {
                                "box": {
                                    "fontsize": 15.0,
                                    "id": "obj-2",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 55.0, 600.0, 24.0 ],
                                    "text": "The class's methods are the object's messages",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 286.0, 100.0 ],
                                    "text": "With no argument the object loads python/default.py, which tap.python~ loads too. Its public methods are messages, called according to their signatures: bang returns the gain, float and int set it, greet prints to the Max console."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 205.0, 278.0, 69.0 ],
                                    "text": "process() is an ordinary method here: process 0.25 calls it, and outputs what it returns — a way to try a tap.python~ class sample by sample."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 5,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 280.0, 330.0, 85.0 ],
                                    "text": "A method named anything answers every message the class has no method or attribute for, with the message's name first: def anything(self, selector: str, *args). Without one, the object doesn't understand it — try nonesuch."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 380.0, 330.0, 69.0 ],
                                    "text": "Names Max and the object keep — dumpout, filechanged, assist and Max's own messages — are not exposed: the console says so, so that you can rename the method."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 90.0, 54.4, 24.0 ],
                                    "text": "bang"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 462.4, 90.0, 92.39999999999999, 24.0 ],
                                    "text": "float 0.5"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 562.8, 90.0, 62.0, 24.0 ],
                                    "text": "int 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 632.8, 90.0, 92.39999999999999, 24.0 ],
                                    "text": "greet max"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 125.0, 115.19999999999999, 24.0 ],
                                    "text": "process 0.25"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 523.2, 125.0, 100.0, 24.0 ],
                                    "text": "nonesuch 1"
                                }
                            },
                            {
                                "box": {
                                    "attr": "gain",
                                    "id": "obj-13",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 165.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "" ],
                                    "patching_rect": [ 400.0, 210.0, 88.0, 24.0 ],
                                    "text": "tap.python"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 260.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 295.0, 120.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-17",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 530.0, 295.0, 180.0, 22.0 ],
                                    "text": "what the method returned"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-13", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-14", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-16", 0 ],
                                    "source": [ "obj-15", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 320.0, 20.0, 88.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p messages"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 0.0, 26.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [
                            {
                                "box": {
                                    "fontface": 1,
                                    "fontsize": 26.0,
                                    "id": "obj-1",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 15.0, 600.0, 38.0 ],
                                    "text": "Threads"
                                }
                            },
                            {
                                "box": {
                                    "fontsize": 15.0,
                                    "id": "obj-2",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 55.0, 600.0, 24.0 ],
                                    "text": "A message runs on the thread it arrives on",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 7,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 300.0, 116.0 ],
                                    "text": "As with any Max object, a message runs on the thread it arrives on, and what the method returns is output before the message returns. From a click or a message box that is Max's main thread; from a metro with Overdrive on, the scheduler thread — where a long method holds the scheduler while it runs."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 7,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 225.0, 284.0, 116.0 ],
                                    "text": "With Scheduler in Audio Interrupt on, and audio running, the scheduler thread is the audio thread: Python then runs on it, where a long method, a reload or another object's Python can interrupt the audio. The object says so in the console, once. deferlow moves the message to the main thread."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 360.0, 330.0, 38.0 ],
                                    "text": "Output is what a method returns: a thread the class starts itself has no outlet to reach."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "toggle",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "int" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 90.0, 24.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "bang" ],
                                    "patching_rect": [ 400.0, 125.0, 81.0, 24.0 ],
                                    "text": "metro 125"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "" ],
                                    "patching_rect": [ 400.0, 210.0, 137.0, 24.0 ],
                                    "text": "tap.python euclid"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 550.0, 165.0, 74.0, 24.0 ],
                                    "text": "deferlow"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "" ],
                                    "patching_rect": [ 550.0, 210.0, 137.0, 24.0 ],
                                    "text": "tap.python euclid"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 260.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 295.0, 140.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 550.0, 260.0, 95.0, 24.0 ],
                                    "text": "prepend set"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 550.0, 295.0, 140.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "linecount": 3,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 335.0, 128.0, 53.0 ],
                                    "text": "on the scheduler thread (Overdrive on)"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 550.0, 335.0, 140.0, 22.0 ],
                                    "text": "on the main thread"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-14", 0 ],
                                    "source": [ "obj-13", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-7", 0 ],
                                    "source": [ "obj-6", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-8", 0 ],
                                    "order": 1,
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-9", 0 ],
                                    "order": 0,
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-11", 0 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-10", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 420.0, 20.0, 81.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p threads"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "newobj",
                    "numinlets": 0,
                    "numoutlets": 0,
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
                        "rect": [ 0.0, 26.0, 800.0, 568.0 ],
                        "default_fontsize": 13.0,
                        "default_fontname": "Ableton Sans Light Regular",
                        "gridsize": [ 5.0, 5.0 ],
                        "gridsnaponopen": 2,
                        "objectsnaponopen": 0,
                        "showontab": 1,
                        "boxes": [],
                        "lines": [],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 520.0, 20.0, 40.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p ?"
                }
            }
        ],
        "lines": [],
        "autosave": 0,
        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
    }
}