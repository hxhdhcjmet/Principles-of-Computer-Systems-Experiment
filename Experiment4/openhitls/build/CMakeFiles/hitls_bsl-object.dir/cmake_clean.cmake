file(REMOVE_RECURSE
  "libhitls_bsl.o"
  "libhitls_bsl.o.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_bsl-object.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
