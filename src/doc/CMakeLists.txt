cmake_minimum_required(VERSION 3.3.0)

# Install documentation that isn't included in the source tree

INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/../doc/"
	DESTINATION "${DOC_INSTALL_DIR}")
