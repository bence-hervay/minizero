#!/bin/bash
set -euo pipefail

env_cmakelists="$(dirname $(readlink -f "$0"))/../minizero/environment/CMakeLists.txt"
support_games=($(awk '/target_include_directories/,/\)/' ${env_cmakelists} | sed 's|/|\n|g' | grep -v -E 'target|environment|PUBLIC|CMAKE_CURRENT_SOURCE_DIR|base|stochastic|)'))

usage() {
	echo "Usage: $0 GAME_TYPE BUILD_TYPE"
	echo ""
	echo "Required arguments:"
	echo "  GAME_TYPE: $(echo ${support_games[@]} | sed 's/ /, /g')"
	echo "  BUILD_TYPE: release(default), debug"
	exit 1
}

build_game() {
	# check arguments is vaild
	local game_type=${1,,}
	local build_type=$2
	[[ " ${support_games[*]} " == *" ${game_type} "* ]] || usage
	[ "${build_type}" == "Debug" ] || [ "${build_type}" == "Release" ] || usage

	local build_dir="build/${game_type}"

	# build
	echo "game type: ${game_type}"
	echo "build type: ${build_type}"
	mkdir -p "${build_dir}"
	cmake -S . -B "${build_dir}" -DCMAKE_BUILD_TYPE="${build_type}" -DGAME_TYPE="${game_type^^}"

	# create git info file
	local git_hash
	local git_short_hash
	local git_info
	git_hash=$(git log -1 --format=%H)
	git_short_hash=$(git describe --abbrev=6 --dirty --always --exclude '*')
	mkdir -p "${build_dir}/git_info"
	git_info=$(echo -e "#pragma once\n\n#define GIT_HASH \"${git_hash}\"\n#define GIT_SHORT_HASH \"${git_short_hash}\"")
	if [ ! -f "${build_dir}/git_info/git_info.h" ] || [ $(diff -q <(echo "${git_info}") <(cat "${build_dir}/git_info/git_info.h") | wc -l 2>/dev/null) -ne 0 ]; then
		echo "${git_info}" > "${build_dir}/git_info/git_info.h"
	fi

	# build
	cmake --build "${build_dir}" --parallel "$(nproc --all)"
}

# add environment settings
git config core.hooksPath .githooks

game_type=${1:-all}
build_type=${2:-release}
build_type=$(echo ${build_type:0:1} | tr '[:lower:]' '[:upper:]')$(echo ${build_type:1} | tr '[:upper:]' '[:lower:]')

if [ "${game_type}" == "all" ]; then
	for game in "${support_games[@]}"
	do
		build_game "${game}" "${build_type}"
	done
else
	build_game "${game_type}" "${build_type}"
fi
