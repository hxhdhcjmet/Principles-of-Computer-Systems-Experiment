file(REMOVE_RECURSE
  "libhitls_auth.pdb"
  "libhitls_auth.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_auth-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
