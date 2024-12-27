#!/usr/bin/env python3
def gen(b):
	prev = -1
	for i in range(7,-1,-1):
		now = (b >> i) & 1

		if prev != now:
			if now:
				print ("    LED_DAT::reset();")
			else:
				print ("    LED_DAT::set();")
			prev = now

		print ('    wait(); LED_CLK::set(); wait(); LED_CLK::reset();')


def led(x, y):
	pattern = (0b00100000 << x) | (0b00000111 ^ y)

	print ("static void led_%sx%i(void) {" % (y, x))
	gen(pattern)
	print ("    kick_off();")
	print ("}")

# Regular RGB
for x in range(3):
	for y in range(8):
		led(x, y)

for y in range(4):
	pattern = 0b00001000 * (0b11 ^ y)

	print ("static void led_%sx%i(void) {" % (y, 3))
	gen(pattern)
	print ("    kick_off();")
	print ("}")
