# Copyright 2019, 2020, Visual Computing Lab, ISTI - Italian National Research Council
# SPDX-License-Identifier: BSL-1.0

# make quiet some portions of cmake
# usage
#
# set(MESSAGE_QUIET ON)
# #everything here will be quiet
# unset(MESSAGE_QUIET)
function(message)
	if (NOT MESSAGE_QUIET)
		_message(${ARGN})
	endif()
endfunction()

function(add_file_format_info_plist)
	cmake_parse_arguments(ARG "" "TARGET;FILE;FORMAT" "" ${ARGN})

	string(TOUPPER ${ARG_FORMAT} FORMAT_UPPER)
	string(TOLOWER ${ARG_FORMAT} FORMAT_LOWER)

	add_custom_command(
		TARGET ${ARG_TARGET}
		POST_BUILD
		COMMAND plutil -insert CFBundleDocumentTypes.0 -xml '<dict><key>CFBundleTypeName</key><string>${FORMAT_UPPER} Relight File</string><key>CFBundleTypeIconFile</key><string>relight.png</string><key>CFBundleTypeRole</key><string>Editor</string><key>LSHandlerRank</key><string>Default</string></dict>' ${ARG_FILE}
		COMMAND plutil -insert CFBundleDocumentTypes.0.CFBundleTypeExtensions -xml '<array/>' ${ARG_FILE}
		COMMAND plutil -insert CFBundleDocumentTypes.0.CFBundleTypeExtensions.0 -xml '<string>${FORMAT_LOWER}</string>' ${ARG_FILE}
		COMMAND plutil -insert CFBundleDocumentTypes.0.CFBundleTypeOSTypes -xml '<array/>' ${ARG_FILE}
		COMMAND plutil -insert CFBundleDocumentTypes.0.CFBundleTypeOSTypes.0 -xml '<string>${FORMAT_UPPER}</string>' ${ARG_FILE}
	)
endfunction()

function(set_additional_settings_info_plist)
	cmake_parse_arguments(ARG "" "TARGET;FILE" "" ${ARGN})
	add_custom_command(
		TARGET ${ARG_TARGET}
		POST_BUILD
		COMMAND plutil -replace NSHighResolutionCapable -bool True ${ARG_FILE}
		COMMAND plutil -replace CFBundleDocumentTypes -xml '<array/>' ${ARG_FILE}
		COMMAND plutil -replace CFBundleIdentifier -string 'com.vcg.relight' ${ARG_FILE}
		#COMMAND plutil -insert NSRequiresAquaSystemAppearance -bool True ${ARG_FILE} || (exit 0)
	)

	add_file_format_info_plist(
		TARGET ${ARG_TARGET}
		FILE ${ARG_FILE}
		FORMAT RELIGHT)
endfunction()
