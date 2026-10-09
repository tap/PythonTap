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
			1040,
			236.0
		],
		"default_fontsize": 12.0,
		"default_fontname": "Arial",
		"gridsize": [
			15.0,
			15.0
		],
		"description": "tap.python.spike-q1-mapping",
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
						96.0
					],
					"text": "tap.python.spike-q1-mapping\n\nQ1 (D11): a patcher with only [tap.python], opened in a FRESH Max (quit Max first). With the package's init/tap.python.txt (max objectfile tap.python tap.python~) the console says 'spike: tap.python registered by tap.python~'s ext_main' and then 'spike: new tap.python'; without it, 'tap.python: No such object'.",
					"linecount": 6
				}
			},
			{
				"box": {
					"id": "obj-2",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						20.0,
						136.0,
						72.0,
						22.0
					],
					"text": "loadbang"
				}
			},
			{
				"box": {
					"id": "obj-3",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"outlettype": [],
					"patching_rect": [
						20.0,
						166.0,
						93.0,
						22.0
					],
					"text": "print spike"
				}
			},
			{
				"box": {
					"id": "obj-4",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 3,
					"outlettype": [
						"",
						"",
						""
					],
					"patching_rect": [
						270.0,
						136.0,
						86.0,
						22.0
					],
					"text": "tap.python",
					"varname": "spike"
				}
			}
		],
		"lines": []
	}
}
