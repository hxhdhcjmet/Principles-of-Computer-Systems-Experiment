file(REMOVE_RECURSE
  "libhitls_pki.a"
  "libhitls_pki.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_pki-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
