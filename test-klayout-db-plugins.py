import klayout.db as kdb

layout = kdb.Layout()

layer1 = layout.layer(kdb.LayerInfo('1/0'))

TOP = layout.create_cell("TOP")

box = kdb.DBox(kdb.DPoint(-4500, -4500), kdb.DPoint(4500, 4500))

TOP.shapes(layer1).insert(box)

layout.write('test.gds')
