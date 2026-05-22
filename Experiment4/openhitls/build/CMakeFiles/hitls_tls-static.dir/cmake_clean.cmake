file(REMOVE_RECURSE
  "libhitls_tls.a"
  "libhitls_tls.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_tls-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
