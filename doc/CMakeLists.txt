# man target
add_custom_target(man ALL)

#
# Install configuration and documentation
#
install(FILES etc/pgprtdbg.conf DESTINATION share/doc/pgprtdbg/etc)

install(DIRECTORY . DESTINATION share/doc/pgagroal FILES_MATCHING PATTERN "*.md" PATTERN "etc" EXCLUDE PATTERN "manual" EXCLUDE)

#
# Generate manual
#
if(generation)
  # Files
  FILE(GLOB PREDS "manual/0?-*.md")
  FILE(GLOB POSTDS "manual/9?-*.md")
  FILE(GLOB TDS "tutorial/??_*.md")
  FILE(GLOB UMDS_PRE "manual/user-0?-*.md")
  FILE(GLOB UMDS_POST "manual/user-1?-*.md")
  FILE(GLOB DMDS "manual/dev-??-*.md")

  # Manuals definitions
  set(MANUAL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/doc")
  get_filename_component(PARENT_DIR ${CMAKE_BINARY_DIR} DIRECTORY)

  add_custom_command(
    OUTPUT ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.pdf
    COMMAND ${PANDOC_EXECUTABLE} -o ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.pdf --from markdown --template eisvogel --listings -N --toc ${PREDS} ${UMDS_PRE} ${TDS} ${UMDS_POST} ${POSTDS}
    DEPENDS ${UMDS}
    COMMENT "Generating User Guide PDF documentation"
  )
  add_custom_command(
    OUTPUT ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.pdf
    COMMAND ${PANDOC_EXECUTABLE} -o ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.pdf --from markdown --template eisvogel --listings -N --toc ${PREDS} ${DMDS} ${POSTDS}
    DEPENDS ${DMDS}
    COMMENT "Generating Developer Guide PDF documentation"
  )

  add_custom_command(
    OUTPUT ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.html
    COMMAND ${PANDOC_EXECUTABLE} -o ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.html -s -f markdown-smart -N --toc -t html5 ${PREDS} ${UMDS_PRE} ${TDS} ${UMDS_POST} ${POSTDS}
    DEPENDS ${UMDS}
    COMMENT "Generating User Guide HTML documentation"
  )
  add_custom_command(
    OUTPUT ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.html
    COMMAND ${PANDOC_EXECUTABLE} -o ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.html -s -f markdown-smart -N --toc -t html5 ${PREDS} ${DMDS} ${POSTDS}
    DEPENDS ${DMDS}
    COMMENT "Generating Developer Guide HTML documentation"
  )

  add_custom_target(
    all_docs ALL
    DEPENDS ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.pdf ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.html ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.pdf ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.html
  )

  install(FILES ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.pdf DESTINATION share/doc/pgprtdbg/manual)
  install(FILES ${MANUAL_OUTPUT_DIR}/pgprtdbg-user-guide.html DESTINATION share/doc/pgprtdbg/manual)
  install(FILES ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.pdf DESTINATION share/doc/pgprtdbg/manual)
  install(FILES ${MANUAL_OUTPUT_DIR}/pgprtdbg-dev-guide.html DESTINATION share/doc/pgprtdbg/manual)

endif()
