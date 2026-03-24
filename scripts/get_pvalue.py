import argparse

def p(x):
  return 1 - (1-x) ** (1/12)

def prob(x):
  return 1 - (1-x) ** 12

parser = argparse.ArgumentParser()
parser.add_argument('-i', action='store_true', default=False)
parser.add_argument('x')
args = parser.parse_args()

if args.i:
  print(prob(float(args.x)))
else:
  print(p(float(args.x)))