
import os
import inspect
import pya

def lineno():
  return inspect.currentframe().f_back.f_lineno

print("Stop 1: " + os.path.basename(pya.Macro.real_path(__file__, lineno())) + ":" + str(pya.Macro.real_line(__file__, lineno())))

# %include a_inc.py

print(f())

print("Stop 2: " + os.path.basename(pya.Macro.real_path(__file__, lineno())) + ":" + str(pya.Macro.real_line(__file__, lineno())))

