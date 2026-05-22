file(REMOVE_RECURSE
  "libhitls_tls.o"
  "libhitls_tls.o.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_tls-object.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
