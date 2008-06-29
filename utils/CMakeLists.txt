
CONFIGURE_FILE(termit.SlackBuild.in ${TERMIT_SOURCE_DIR}/utils/termit.SlackBuild @ONLY)
CONFIGURE_FILE(termit.ebuild ${TERMIT_SOURCE_DIR}/utils/termit-${TERMIT_PACKAGE_VERSION}.ebuild COPY_ONLY)

