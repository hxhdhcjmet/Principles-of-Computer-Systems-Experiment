file(REMOVE_RECURSE
  "libhitls_auth.a"
  "libhitls_auth.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_auth-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
