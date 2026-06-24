s = "08:XYZ:ping"
x = 0
for b in s.encode():
    x ^= b
print("checksum %s = %02X" % (s, x))
