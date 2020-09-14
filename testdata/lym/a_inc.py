
import os

def f():
  return("f: " + os.path.basename(pya.Macro.real_path(__file__, lineno())) + ":" + str(pya.Macro.real_line(__file__, lineno())))

