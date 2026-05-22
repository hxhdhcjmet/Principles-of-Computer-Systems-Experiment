file(REMOVE_RECURSE
  "libhitls_crypto.pdb"
  "libhitls_crypto.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_crypto-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
