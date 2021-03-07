#!/bin/bash

HOST_XAUTH=$(xauth info | grep "Authority file" | awk '{print $3}')
REPO_ROOT=$(git rev-parse --show-toplevel)
# Set your default values
IMAGE_NAME=''
DOCKER_DEBUG_PARAM=''
IN=''

while [[ "$#" -gt 0 ]]; do
	case $1 in
		-g|--gdb_env) DOCKER_DEBUG_PARAM='--cap-add=SYS_PTRACE --security-opt seccomp=unconfined' ;;
		-i|--in) IN="$2"; shift ;;
		-c|--container_name) IMAGE_NAME="$2"; shift ;;
		-h|--help)
			echo "-g / --gdb-env          flag to allow gdb use inside the container"
			echo "-i / --in               the to be send to the container"
			echo "-c / --container_name   the container image name"
			exit 0
			;;
		*) echo "Unknown parameter passed: $1"; exit 1 ;;
	esac
	shift
done

if [ "$IMAGE_NAME" == '' ]; then
	echo "Error: No image name passed, use -c / --container_name to inform the container image name/id"
	exit -1
fi

if [ "$IN" == '' ]; then
	echo "Error: No command passed, use -c / --container_name the" \
		 "command that will be executed inside of container '$IMAGE_NAME'"
	exit -1
fi

# docker run : run a command to a container. In this case the '$*' is the complement
#				of the make command of this specific container
# --rm : Delete the container after the exit
# -a <Stream> : Bind the stream of container to the host
# -v <Host Dir>:<Container Dir> : Bind the volume to the container(Both absolute PATHS)
# -e <Var> : Set enviroment variable to the container
# --net=host : Set the container network the same as the host
# --device : bind the host device to the container

sudo docker run --rm -a STDOUT -a STDERR -v $REPO_ROOT:/home/mainCraft/VK_App/ -e DISPLAY \
	--net=host --device=/dev/dri --device=/dev/vga_arbiter $DOCKER_DEBUG_PARAM \
	-v $HOST_XAUTH:/root/.Xauthority:rw --name vulkan_container $IMAGE_NAME $IN

