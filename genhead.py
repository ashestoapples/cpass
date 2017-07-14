from sys import argv

if len(argv) < 2 or argv[1] == '-h':
	print("Usage: \"chead src.c --init\"\n --init is optional, it writes a full header file with include guards")
else:
	functions = []
	stack = []
	with open(argv[1], 'r') as fp:
		lines = fp.readlines()
		for i in range(len(lines)):
			
