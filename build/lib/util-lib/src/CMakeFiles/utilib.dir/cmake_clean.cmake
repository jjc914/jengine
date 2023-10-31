file(REMOVE_RECURSE
  "libutilib.dylib"
  "libutilib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/utilib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
