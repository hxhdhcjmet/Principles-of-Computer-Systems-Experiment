file(REMOVE_RECURSE
  "libhitls_pki.o"
  "libhitls_pki.o.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/hitls_pki-object.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
