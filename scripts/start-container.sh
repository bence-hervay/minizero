#!/bin/bash
set -e

usage()
{
	echo "Usage: $0 [OPTION]..."
	echo ""
	echo "Optional arguments:"
	echo "  -h, --help     Give this help list"
	echo "      --image    Select the image name of the container"
	echo "      --rebuild-image  Rebuild the default local image before starting"
	echo "  -v, --volume   Bind mount a volume into the container"
	echo "      --name     Assign a name to the container"
	echo "  -d, --detach   Run container in background and print container ID"
	echo "  -H, --history  Record the container bash history"
	exit 1
}

script_dir=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
default_image_name=minizero:local
image_name=${default_image_name}
rebuild_image=false
container_tool=$(basename $(which podman || which docker) 2>/dev/null)
if [[ ! $container_tool ]]; then
	echo "Neither podman nor docker is installed." >&2
	exit 1
fi
container_volume="-v .:/workspace"
container_arguments=""
record_history=false
while :; do
	case $1 in
		-h|--help) shift; usage
		;;
		--image) shift; image_name=${1}
		;;
		--rebuild-image) rebuild_image=true
		;;
		-v|--volume) shift; container_volume="${container_volume} -v ${1}"
		;;
		--name) shift; container_arguments="${container_arguments} --name ${1}"
		;;
		-d|--detach) container_arguments="${container_arguments} -d"
		;;
		-H|--history) record_history=true
		;;
		"") break
		;;
		*) echo "Unknown argument: $1"; usage
		;;
	esac
	shift
done

if [[ ${image_name} == ${default_image_name} ]] && { [[ ${rebuild_image} == true ]] || ! ${container_tool} image inspect "${image_name}" >/dev/null 2>&1; }; then
	echo "Building local image ${image_name} from ${repo_root}/docker/Dockerfile ..."
	${container_tool} build -t "${image_name}" -f "${repo_root}/docker/Dockerfile" "${repo_root}/docker"
fi

# check GPU environment
if ! command -v nvidia-smi &> /dev/null; then
	echo "Error: nvidia-smi not found. Please ensure NVIDIA drivers are installed and available in PATH." >&2
	exit 1
fi

if ! command -v nvidia-container-cli &> /dev/null; then
	echo "Error: 'nvidia-container-cli' not found. NVIDIA Container Toolkit is likely not installed." >&2
	echo "Visit https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html" >&2
	exit 1
fi

if [ "$record_history" = true ]; then
	history_dir=".container_root"
	# if the history directory is not exist, create it and initialize the history directory
	if [ ! -d ${history_dir} ]; then
		mkdir -p ${history_dir}
		# start container with the history directory and copy the history to the history directory
		$container_tool run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --rm -it -v "${history_dir}:/container_root" ${image_name} /bin/bash -c "cp -r /root/. /container_root && touch /container_root/.bash_history && exit"
	fi
	# add the history directory to the container volume
	container_volume="${container_volume} -v ${history_dir}:/root"
fi

container_arguments=$(echo ${container_arguments} | xargs)

if [ ${container_tool} = "podman" ]; then
	runtime_check_cmd='if [ -x /workspace/scripts/check-gpu-runtime.sh ]; then /workspace/scripts/check-gpu-runtime.sh || true; fi'
	git_safe_cmd="${runtime_check_cmd} && git config --global --add safe.directory /workspace && exec bash"
	container_arguments="${container_arguments} -e container=${container_tool}"
	echo "$container_tool run ${container_arguments} --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=host --ipc=host --rm -it ${container_volume} ${image_name} bash -c \"${git_safe_cmd}\""
	$container_tool run ${container_arguments} --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=host --ipc=host --rm -it ${container_volume} ${image_name} bash -c "${git_safe_cmd}"
elif [ ${container_tool} = "docker" ]; then
	# manually mount GPU devices into Docker to resolve 'Failed to Initialize NVML: Unknown Error'
	device_args="--gpus all"
	for path in /dev/nvidia*; do
		if [ -c ${path} ]; then
			device_args+=" --device=${path}"
		fi
	done
	# add Git safe directory
	runtime_check_cmd='if [ -x /workspace/scripts/check-gpu-runtime.sh ]; then /workspace/scripts/check-gpu-runtime.sh || true; fi'
	git_safe_cmd="${runtime_check_cmd} && git config --global --add safe.directory /workspace && exec bash"
	# add argument
	container_arguments="${container_arguments} -e container=${container_tool}"
	echo "$container_tool run ${container_arguments} ${device_args} --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=host --ipc=host --rm -it ${container_volume} ${image_name} bash -c \"${git_safe_cmd}\""
	$container_tool run ${container_arguments} ${device_args} --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=host --ipc=host --rm -it ${container_volume} ${image_name} bash -c "$git_safe_cmd"
fi
