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
        "rect": [ 100.0, 100.0, 800.0, 680.0 ],
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
                        "rect": [ 100.0, 126.0, 800.0, 654.0 ],
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
                                    "text": "tap.python~"
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
                                    "text": "Process audio with a Python class",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 330.0, 69.0 ],
                                    "text": "[tap.python~ name] loads python/name.py from this package and runs the class name in it as an audio object. With no argument it loads python/default.py: open it to follow along."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 170.0, 308.0, 100.0 ],
                                    "text": "Its typed fields are attributes (gain), its public methods are messages called according to their signatures (greet, float, int, bang), and its process() method runs on the signal — here once per sample; the numpy tab has the form that runs once per vector."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 285.0, 260.0, 100.0 ],
                                    "text": "Save the .py file and the object loads it again, keeping its attribute values, and the audio carries on with the new code. A mistake prints its traceback to the Max console, and the object outputs silence until a save fixes it."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 400.0, 304.0, 69.0 ],
                                    "text": "@mode worker runs process() on a thread of its own, @latency milliseconds (30) behind the audio, so that a reload or a message never holds the audio up."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 90.0, 74.0, 24.0 ],
                                    "text": "saw~ 110"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 480.0, 90.0, 74.0, 24.0 ],
                                    "text": "saw~ 221"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 125.0, 60.0, 24.0 ],
                                    "text": "*~ 0.5"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 160.0, 92.39999999999999, 24.0 ],
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
                                    "patching_rect": [ 500.4, 160.0, 92.39999999999999, 24.0 ],
                                    "text": "float 0.5"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 600.8, 160.0, 62.0, 24.0 ],
                                    "text": "int 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 670.8, 160.0, 107.6, 24.0 ],
                                    "text": "filechanged"
                                }
                            },
                            {
                                "box": {
                                    "attr": "gain",
                                    "id": "obj-14",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 195.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 235.0, 95.0, 24.0 ],
                                    "text": "tap.python~"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 490.0, 235.0, 260.0, 38.0 ],
                                    "text": "filechanged reloads at once; saving the file does it for you"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-17",
                                    "maxclass": "scope~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 490.0, 280.0, 150.0, 70.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-18",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 280.0, 60.0, 24.0 ],
                                    "text": "*~ 0.2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-19",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 315.0, 45.0, 45.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-20",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "bang" ],
                                    "patching_rect": [ 660.0, 280.0, 151.0, 24.0 ],
                                    "text": "metro 250 @active 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-21",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "int" ],
                                    "patching_rect": [ 660.0, 310.0, 102.0, 24.0 ],
                                    "text": "adstatus cpu"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-22",
                                    "maxclass": "number",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 660.0, 340.0, 60.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-23",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 725.0, 340.0, 60.0, 22.0 ],
                                    "text": "CPU %"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
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
                                    "destination": [ "obj-17", 0 ],
                                    "order": 0,
                                    "source": [ "obj-15", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-18", 0 ],
                                    "order": 1,
                                    "source": [ "obj-15", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-19", 1 ],
                                    "order": 0,
                                    "source": [ "obj-18", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-19", 0 ],
                                    "order": 1,
                                    "source": [ "obj-18", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-21", 0 ],
                                    "source": [ "obj-20", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-22", 0 ],
                                    "source": [ "obj-21", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-9", 0 ],
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-9", 0 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
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
                        "rect": [ 0.0, 26.0, 800.0, 654.0 ],
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
                                    "text": "Per vector, with numpy"
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
                                    "text": "One call per signal vector instead of one per sample",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 284.0, 69.0 ],
                                    "text": "process(self, x: float) -> float is called once per sample: simplest to write, but every line of Python in it runs tens of thousands of times a second."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 170.0, 296.0, 100.0 ],
                                    "text": "process(self, x: np.ndarray) -> np.ndarray is called once per signal vector, with the vector as a numpy array, and numpy then works on the whole of it at C speed: the form for anything with real work in it. numpy_gain.py is default.py's gain written that way."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 285.0, 330.0, 69.0 ],
                                    "text": "Choose one with the message boxes and watch the CPU meter. The PythonTap book has the measurements, per sample and per vector, at 48 and 96 kHz."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 365.0, 330.0, 22.0 ],
                                    "text": "Turn on audio with the speaker."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 90.0, 74.0, 24.0 ],
                                    "text": "saw~ 110"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 165.0, 151.0, 24.0 ],
                                    "text": "tap.python~ default"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 570.0, 165.0, 172.0, 24.0 ],
                                    "text": "tap.python~ numpy_gain"
                                }
                            },
                            {
                                "box": {
                                    "attr": "gain",
                                    "id": "obj-10",
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
                                    "attr": "gain",
                                    "id": "obj-11",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 570.0, 130.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 210.0, 40.0, 24.0 ],
                                    "text": "1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 448.0, 210.0, 40.0, 24.0 ],
                                    "text": "2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 500.0, 210.0, 170.0, 22.0 ],
                                    "text": "per sample, per vector"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 3,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 250.0, 109.0, 24.0 ],
                                    "text": "selector~ 2 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 290.0, 60.0, 24.0 ],
                                    "text": "*~ 0.2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-17",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 325.0, 45.0, 45.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-18",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "bang" ],
                                    "patching_rect": [ 570.0, 290.0, 151.0, 24.0 ],
                                    "text": "metro 250 @active 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-19",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "int" ],
                                    "patching_rect": [ 570.0, 320.0, 102.0, 24.0 ],
                                    "text": "adstatus cpu"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-20",
                                    "maxclass": "number",
                                    "numinlets": 1,
                                    "numoutlets": 2,
                                    "outlettype": [ "", "bang" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 570.0, 350.0, 60.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-21",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 635.0, 350.0, 60.0, 22.0 ],
                                    "text": "CPU %"
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-8", 0 ],
                                    "source": [ "obj-10", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-9", 0 ],
                                    "source": [ "obj-11", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-13", 0 ]
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
                                    "destination": [ "obj-17", 1 ],
                                    "order": 0,
                                    "source": [ "obj-16", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-17", 0 ],
                                    "order": 1,
                                    "source": [ "obj-16", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-19", 0 ],
                                    "source": [ "obj-18", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-20", 0 ],
                                    "source": [ "obj-19", 0 ]
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
                                    "destination": [ "obj-15", 1 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 2 ],
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
                    "text": "p numpy"
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
                        "rect": [ 0.0, 26.0, 800.0, 654.0 ],
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
                                    "text": "A filter that knows the sample rate"
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
                                    "text": "prepare(sample_rate, vector_size)",
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
                                    "patching_rect": [ 20.0, 90.0, 270.0, 100.0 ],
                                    "text": "allpass.py is a Schroeder allpass filter: it passes every frequency at the same level and shifts their phase, so mixed with the dry signal it notches the spectrum. Its delay is an attribute in milliseconds, which it turns into samples."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 205.0, 302.0, 100.0 ],
                                    "text": "For that it needs the sample rate: an optional method prepare(self, sample_rate: float, vector_size: int) -> None is called before the object processes any audio, again whenever the rate or the vector size changes, and on every reload before the new code runs."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 325.0, 316.0, 69.0 ],
                                    "text": "Its fields are attrs fields with validators: alpha must stay strictly between -1 and 1. Try alpha 1 — the validator's error prints to the console, and alpha keeps its value."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "linecount": 2,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 415.0, 332.0, 38.0 ],
                                    "text": "numpy_allpass.py computes exactly the same, a vector at a time."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 90.0, 60.0, 24.0 ],
                                    "text": "noise~"
                                }
                            },
                            {
                                "box": {
                                    "attr": "delay",
                                    "id": "obj-8",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 400.0, 125.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "attr": "alpha",
                                    "id": "obj-9",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 560.0, 125.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 560.0, 160.0, 77.19999999999999, 24.0 ],
                                    "text": "alpha 1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 645.2, 160.0, 62.0, 24.0 ],
                                    "text": "clear"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 195.0, 151.0, 24.0 ],
                                    "text": "tap.python~ allpass"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 235.0, 40.0, 24.0 ],
                                    "text": "+~"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 445.0, 235.0, 150.0, 22.0 ],
                                    "text": "with the dry signal"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 275.0, 60.0, 24.0 ],
                                    "text": "*~ 0.1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-16",
                                    "maxclass": "ezdac~",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 310.0, 45.0, 45.0 ]
                                }
                            }
                        ],
                        "lines": [
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
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
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-15", 0 ],
                                    "source": [ "obj-13", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-16", 1 ],
                                    "order": 0,
                                    "source": [ "obj-15", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-16", 0 ],
                                    "order": 1,
                                    "source": [ "obj-15", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
                                    "order": 1,
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-13", 1 ],
                                    "order": 0,
                                    "source": [ "obj-7", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
                                    "source": [ "obj-8", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-12", 0 ],
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
                    "text": "p allpass"
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
                        "rect": [ 0.0, 26.0, 800.0, 654.0 ],
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
                                    "text": "Many channels"
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
                                    "text": "mc., and a class with several inputs",
                                    "textcolor": [ 0.976470588235294, 0.733333333333333, 0.258823529411765, 1.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-3",
                                    "linecount": 9,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 90.0, 284.0, 147.0 ],
                                    "text": "Wrap the object in mc. to run one instance per channel of a multichannel signal: [mc.tap.python~ numpy_gain] here, over two channels. The class's attributes and messages are each instance's own, so the MC wrapper passes them on through its own messages: setvalue, an instance's number and the message, for one; applyvalues, the name and a value for each, for all."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-4",
                                    "linecount": 6,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 255.0, 294.0, 100.0 ],
                                    "text": "Or write one class with several channels: process()'s parameters are the object's signal inlets and its return hint its outlets. stereo_width.py takes left and right, returns tuple[np.ndarray, np.ndarray], and gets two of each."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-5",
                                    "linecount": 4,
                                    "maxclass": "comment",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 20.0, 370.0, 284.0, 69.0 ],
                                    "text": "A save that changes process()'s inputs or outputs changes the object's inlets and outlets to match, keeping the patch cords of those that stay."
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-6",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 90.0, 74.0, 24.0 ],
                                    "text": "saw~ 110"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-7",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 480.0, 90.0, 74.0, 24.0 ],
                                    "text": "saw~ 165"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-8",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "multichannelsignal" ],
                                    "patching_rect": [ 400.0, 125.0, 88.0, 24.0 ],
                                    "text": "mc.pack~ 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-9",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 400.0, 160.0, 214.0, 24.0 ],
                                    "text": "applyvalues gain 0.5 0.25"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-10",
                                    "maxclass": "message",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "patching_rect": [ 622.0, 160.0, 168.4, 24.0 ],
                                    "text": "setvalue 2 gain 0.1"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-11",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "multichannelsignal" ],
                                    "patching_rect": [ 400.0, 195.0, 193.0, 24.0 ],
                                    "text": "mc.tap.python~ numpy_gain"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-12",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "multichannelsignal" ],
                                    "patching_rect": [ 400.0, 230.0, 81.0, 24.0 ],
                                    "text": "mc.*~ 0.2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-13",
                                    "maxclass": "newobj",
                                    "numinlets": 1,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 265.0, 95.0, 24.0 ],
                                    "text": "mc.dac~ 1 2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-14",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 330.0, 74.0, 24.0 ],
                                    "text": "saw~ 110"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-15",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 480.0, 330.0, 88.0, 24.0 ],
                                    "text": "saw~ 110.5"
                                }
                            },
                            {
                                "box": {
                                    "attr": "width",
                                    "id": "obj-16",
                                    "maxclass": "attrui",
                                    "numinlets": 1,
                                    "numoutlets": 1,
                                    "outlettype": [ "" ],
                                    "parameter_enable": 0,
                                    "patching_rect": [ 570.0, 330.0, 150.0, 24.0 ]
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-17",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 2,
                                    "outlettype": [ "signal", "signal" ],
                                    "patching_rect": [ 400.0, 370.0, 186.0, 24.0 ],
                                    "text": "tap.python~ stereo_width"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-18",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 400.0, 405.0, 60.0, 24.0 ],
                                    "text": "*~ 0.2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-19",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 1,
                                    "outlettype": [ "signal" ],
                                    "patching_rect": [ 480.0, 405.0, 60.0, 24.0 ],
                                    "text": "*~ 0.2"
                                }
                            },
                            {
                                "box": {
                                    "id": "obj-20",
                                    "maxclass": "newobj",
                                    "numinlets": 2,
                                    "numoutlets": 0,
                                    "patching_rect": [ 400.0, 440.0, 74.0, 24.0 ],
                                    "text": "dac~ 1 2"
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
                                    "destination": [ "obj-13", 0 ],
                                    "source": [ "obj-12", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-17", 0 ],
                                    "source": [ "obj-14", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-17", 1 ],
                                    "source": [ "obj-15", 0 ]
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
                                    "destination": [ "obj-18", 0 ],
                                    "source": [ "obj-17", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-19", 0 ],
                                    "source": [ "obj-17", 1 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-20", 0 ],
                                    "source": [ "obj-18", 0 ]
                                }
                            },
                            {
                                "patchline": {
                                    "destination": [ "obj-20", 1 ],
                                    "source": [ "obj-19", 0 ]
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
                                    "destination": [ "obj-8", 1 ],
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
                                    "destination": [ "obj-11", 0 ],
                                    "source": [ "obj-9", 0 ]
                                }
                            }
                        ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ],
                        "bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ]
                    },
                    "patching_rect": [ 320.0, 20.0, 46.0, 24.0 ],
                    "saved_object_attributes": {
                        "editing_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "fontname": "Ableton Sans Light Regular",
                        "fontsize": 13.0,
                        "locked_bgcolor": [ 0.109803921568627, 0.109803921568627, 0.109803921568627, 1.0 ],
                        "textcolor": [ 0.847058823529412, 0.847058823529412, 0.847058823529412, 1.0 ]
                    },
                    "text": "p mc"
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
                        "rect": [ 0.0, 26.0, 800.0, 654.0 ],
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
                    "patching_rect": [ 420.0, 20.0, 40.0, 24.0 ],
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