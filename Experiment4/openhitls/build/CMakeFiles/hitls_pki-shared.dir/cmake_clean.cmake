file(REMOVE_RECURSE
  "libhitls_pki.pdb"
  "libhitls_pki.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_pki-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
