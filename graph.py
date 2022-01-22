import igraph
import sys

# load data into a graph

inputs = [
	"MyObjs",
	"OpObjs",

	"DWthr",
	"AWthr",
	"WthrCtr",

	"MyDir",
	"MyOpRP",
	"MyBCornD",
	"MyFCornD",
	"MyBGrndD",
	"MyAc",
	"MyAcBId",
	"MyAnCtr",
	"MyAnSubF",
	"MyFrame",
	"MyCDmg",
	"MyCLmt",
	"MyAir",
	"MyHP",
	"MyADCnt",
	"MySP",
	"MySPM",
	"MyUnT",
	"MyHealC",
	"MySOR",
	"MyScore",
	"MyCrd1",
	"MyCrd2",
	"MyCrd3",
	"MyCrd4",
	"MyCrd5",
	"MyCrdG",
	"MySkl1",
	"MySkl2",
	"MySkl3",
	"MySkl4",
	"MySkl5",
	"MySkl6",
	"MySkl7",
	"MySkl8",
	"MySkl9",
	"MySklA",
	"MySklB",
	"MySklC",
	"MySklD",
	"MySklE",
	"MySklF",
	"MyFnLvl",
	"MyDInvT",
	"MyArmT",
	"MyArmHP",
	"MyVampT",
	"MyPhilT",
	"MyWorlT",
	"MyPST",
	"MyOrrT",
	"MyMPPT",
	"MyKanCd",
	"MySuwCd",

	"OpDir",
	"OpOpRP",
	"OpBCornD",
	"OpFCornD",
	"OpBGrndD",
	"OpAc",
	"OpAcBId",
	"OpAnCtr",
	"OpAnSubF",
	"OpFrame",
	"OpCDmg",
	"OpCLmt",
	"OpAir",
	"OpHP",
	"OpADCnt",
	"OpSP",
	"OpSPM",
	"OpUnT",
	"OpHealC",
	"OpSOR",
	"OpScore",
	"OpCrd1",
	"OpCrd2",
	"OpCrd3",
	"OpCrd4",
	"OpCrd5",
	"OpCrdG",
	"OpSkl1",
	"OpSkl2",
	"OpSkl3",
	"OpSkl4",
	"OpSkl5",
	"OpSkl6",
	"OpSkl7",
	"OpSkl8",
	"OpSkl9",
	"OpSklA",
	"OpSklB",
	"OpSklC",
	"OpSklD",
	"OpSklE",
	"OpSklF",
	"OpFnLvl",
	"OpDInvT",
	"OpArmT",
	"OpArmHP",
	"OpVampT",
	"OpPhilT",
	"OpWorlT",
	"OpPST",
	"OpOrrT",
	"OpMPPT",
	"OpKanCd",
	"OpSuwCd",
	
	"Rnd1",
	"Rnd2",
	"Rnd3",
	"Rnd4",
	"FrmID"
]
outputs = [
	"Stnd",
	"Forw",
	"Back",
	"Lblc",
	"Crou",
	"Fjmp",
	"Bjmp",
	"Jump",
	"Fhj",
	"Bhj",
	"Hj",
	"D6",
	"D4",
	"Jd1",
	"Jd2",
	"Jd3",
	"Jd4",
	"Jd6",
	"Jd7",
	"Jd8",
	"Jd9",
	"SwCrd1",
	"SwCrd2",
	"SwCrd3",
	"SwCrd4",
	"Sc",
	"Skill",
	"236b",
	"236c",
	"623b",
	"623c",
	"214b",
	"214c",
	"421b",
	"421c",
	"22b",
	"22c",
	"be1",
	"be2",
	"be4",
	"be6",
	"2b",
	"3b",
	"4b",
	"5b",
	"6b",
	"66b",
	"1c",
	"2c",
	"3c",
	"4c",
	"5c",
	"6c",
	"66c",
	"1a",
	"2a",
	"3a",
	"5a",
	"6a",
	"8a",
	"66a",
	"4a"
]
d = {}
t = {}
p = {}

file = open(sys.argv[1], "rb")
data = file.read()
file.close()
file = open("net.txt", "w")


for i in range(0, len(data), 8):
	input  = int.from_bytes(data[i:i+2], byteorder="little")
	output = int.from_bytes(data[i+2:i+4], byteorder="little")
	wgt	= int.from_bytes(data[i+4:i+6], byteorder="little")

	input  &= 0x81FF
	output &= 0x81FF
	if input & 0x8000:
		input &= 0x7FFF
		if input >= len(inputs):
			continue
		input = inputs[input]
	elif input > 500:
		continue
	else:
		input = "N{:X}".format(input)
	if output & 0x8000:
		output &= 0x7FFF
		if output >= len(outputs):
			continue
		output = outputs[output]
		o = True
	elif output > 500:
		continue
	else:
		output = "N{:X}".format(output)
	if wgt & 0x8000:
		wgt -= 0x10000
	if not wgt:
		continue
	if input not in d:
		d[input] = {}
	if output not in d[input]:
		d[input][output] = []
	d[input][output].append(wgt)

qzd = True
while qzd:
	print()
	qzd = False
	for key, value in d.items():
		if key not in p and key not in inputs:
			continue
		for i, k in value.items():
			if i not in p:
				print(f"{i} is connected to input {key}")
				qzd = True
				p[i] = True
				break

qzd = True
while qzd:
	print()
	qzd = False
	for key, value in d.items():
		if key not in p and key not in inputs:
			continue
		if key in t:
			continue
		for i, k in value.items():
			if i in outputs:
				print(f"{key} is connected to output {i}")
				qzd = True
				t[key] = True
				break
			if i in t:
				print(f"{key} is connected to neuron {i}")
				qzd = True
				t[key] = True
				break

print(len(p.keys()), p)
print(len(t.keys()), t)
for key, value in d.items():
	if key in t:
		for i, k in value.items():
			if i in t or (i in outputs and i in p):
				for j in k:
					file.write("{} {} {}\n".format(key, i, str(j)))

file.close()

g = igraph.Graph.Read_Ncol('net.txt', names=True, weights=True)
for v in g.vs:
	v['size'] = 35
	v['label'] = v['name']
	if v['name'] in inputs:
		v['color'] = 'lightblue'
	elif v['name'] in outputs:
		v['color'] = 'lightpink'
	else:
		v['color'] = 'lightgrey'

# convert edge weights to color and size
for e in g.es:
	#print(e['weight'])
	if e['weight'] < 0:
		e['color'] = 'lightcoral'
	elif e['weight'] == 0:
		e['color'] = 'grey'
	else:
		e['color'] = 'green'

	width = abs(e['weight'])
	e['width'] = 1 + 1.25 * (width / 65535)


# plot graph

print(len(g.vs))

if len(g.vs) < 6:
	bbox = (300,300)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 12:
	bbox = (400,400)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 18:
	bbox = (500,500)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 24:
	bbox = (520,520)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 26:
	bbox = (800,800)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 50:
	bbox = (1000,1000)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 130:
	bbox = (1200,1000)
	layout = 'fruchterman_reingold'
elif len(g.vs) < 150:
	bbox = (4000,4000)
	layout = 'fruchterman_reingold'
	for v in g.vs:
		v['size'] = v['size'] * 1.5
elif len(g.vs) < 200:
	bbox = (4000,4000)
	layout = 'kamada_kawai'
	for v in g.vs:
		v['size'] = v['size'] * 2
else:
	bbox = (8000,8000)
	layout = 'fruchterman_reingold'

igraph.plot(g, sys.argv[2], edge_curved=True, bbox=bbox, margin=64, layout=layout)