file(REMOVE_RECURSE
  "libhitls_crypto.o"
  "libhitls_crypto.o.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_crypto-object.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
