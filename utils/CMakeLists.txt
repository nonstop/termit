
# Slackware SlackBuild
CONFIGURE_FILE(termit.SlackBuild.in ${TERMIT_SOURCE_DIR}/utils/termit.SlackBuild @ONLY)
# Gentoo ebuild
CONFIGURE_FILE(termit.ebuild ${TERMIT_SOURCE_DIR}/utils/termit-${TERMIT_PACKAGE_VERSION}.ebuild COPY_ONLY)
# Fedora spec
CONFIGURE_FILE(termit.spec.in ${TERMIT_SOURCE_DIR}/utils/termit.spec @ONLY)

