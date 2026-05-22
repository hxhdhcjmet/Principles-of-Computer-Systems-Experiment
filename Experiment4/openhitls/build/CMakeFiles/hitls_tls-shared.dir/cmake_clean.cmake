file(REMOVE_RECURSE
  "libhitls_tls.pdb"
  "libhitls_tls.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_tls-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
