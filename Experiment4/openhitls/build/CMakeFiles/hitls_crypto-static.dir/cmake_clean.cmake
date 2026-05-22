file(REMOVE_RECURSE
  "libhitls_crypto.a"
  "libhitls_crypto.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_crypto-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
