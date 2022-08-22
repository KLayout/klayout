Author: Thomas Ferreira de Lima
email: thomas@tlima.me

## Notes
To use the stubgen script for the three main modules, run the following from the root folder of the repository:
`$  python ./src/pymod/stubgen.py db >! src/pymod/distutils_src/klayout/dbcore.pyi`
`$  python ./src/pymod/stubgen.py rdb >! src/pymod/distutils_src/klayout/rdbcore.pyi`
`$  python ./src/pymod/stubgen.py tl >! src/pymod/distutils_src/klayout/tlcore.pyi`

To compare the generated stubs with a python self-inspection of the klayout module, try the following:
Navigate to `./src/pymod/distutils_src`.
Run, for example:
`$ stubtest klayout.tlcore`

TODO:
- [ ] Integrate above scripts with CI
## Old notes
CHECKLIST:
- [x] 1. Use klayout.tl to inspect all classes and methods in pya.
- [x] 2. Figure out last few bugs.
    - DPoint has a method with "=" when it should have been "*=". There must be an issue with the gsiDeclInternal algorithms.
    - Some inner classes, e.g. LogicalOp inside CompoundRegionOperationNode are not returning
- [x] 3. Manually check and compare to mypy's output.
    - Looks good, but there are a few discrepancies between actual python module and stubs. Namely, deprecated methods were not included in the stub. The opposite is sometimes true as well, though for newer, experimental classes e.g. `klayout.dbcore.GenericDeviceCombiner.combine_devices`.