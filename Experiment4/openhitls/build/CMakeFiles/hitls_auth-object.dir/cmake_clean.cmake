file(REMOVE_RECURSE
  "libhitls_auth.o"
  "libhitls_auth.o.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_auth-object.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
