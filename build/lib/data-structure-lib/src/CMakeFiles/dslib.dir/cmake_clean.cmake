file(REMOVE_RECURSE
  "libdslib.dylib"
  "libdslib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/dslib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
