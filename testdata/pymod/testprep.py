
import os

# Whatever drove these people to drop PATH support for Windows in Python 3.8 (bpo-36085)
# they at least had mercy and let us emulate PATH through os.add_dll_directory.

external_dll_path = os.getenv("KLAYOUT_PYMOD_DLL_PATH")
if ("add_dll_directory" in os.__dict__) and external_dll_path is not None:
  for p in external_dll_path.split(os.pathsep):
    os.add_dll_directory(p)

