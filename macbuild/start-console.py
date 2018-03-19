#!./klayout -b -r
import readline
import code
variables = globals().copy()
variables.update(locals())
shell = code.InteractiveConsole(variables)
self.write("Python %s on %s\n%s\n(%s)\n" %
           (sys.version, sys.platform, cprt,
            "KLayout Python Console"))
shell.interact(banner)
