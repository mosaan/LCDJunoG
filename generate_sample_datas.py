from random import randint

filename = "juno_sigrok_12m_10g2.csv"  # no duplicates

def convert_to_c_code(sample_size: int):
  # Time,D0,D1,D2,D3,D4,D5,D6,D7,D8,D9,D10,D11,D12,D13,D14,D15
  # >Time,D0,D1,D2,D3,D4,D5,D6,D7,WE,RS,CS1,CS2,???,???,???,???
  # 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  # ...
  with open(filename) as f:
    with open("src/test_datas.c", "w") as out:
      out.write("#include <stdint.h>\n")
      out.write("const uint16_t sample_datas[] = {\n")
      count = 0
      for i in f:
        ss = i.rstrip().split(",")
        if ss[0] == "Time":
          continue

        rs = int(ss[9])
        cs1 = int(ss[11])
        cs2 = int(ss[12])
        prefix = rs << 2 | cs1 << 1 | cs2
        data = 0
        for bit, value in enumerate(ss[1:9]):
          data |= int(value) << bit
        # packed 16bit data. like: 0b0000_0RCc_DDDD_DDDD. R=RS, C=CS1, c=CS2, D=data
        whole = prefix << 8 | data
        if whole == 0:
          continue  # skip empty data
        out.write("0x%04x,\n" % whole)
        count += 1
        # limit to 50k samples (500k * 16bit = 1Mbytes)
        if count >= sample_size:
          break
      out.write("};\n")
      actual_size = count
      
  with open("src/test_datas.h", "w") as f:
    f.writelines([
      "#pragma once\n",
      "#include <stdint.h>\n",
      "#define RS_BIT %d\n" % 10,
      "#define CS1_BIT %d\n" % 9,
      "#define CS2_BIT %d\n" % 8,
      "#define SAMPLE_SIZE %d\n" % actual_size,
      "extern const uint16_t sample_datas[SAMPLE_SIZE];\n",
    ])


if __name__ == "__main__":
  import sys
  sample_size = 500 * 1024
  if len(sys.argv) > 1 and sys.argv[1].isdigit():
    sample_size = int(sys.argv[1])
  convert_to_c_code(sample_size)
