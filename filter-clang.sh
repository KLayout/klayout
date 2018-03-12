filter () { 
    sed -E 's/^\/Applications\/Xcode.app\/Contents\/Developer\/Toolchains\/XcodeDefault\.xctoolchain\/usr\/bin\/clang\+\+ -c[[:space:]]+([^[:space:]]+[[:space:]]+)*/Compiling \1/' 
}
