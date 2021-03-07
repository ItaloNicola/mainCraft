FROM debian:testing-slim AS vulkan-sdk-builder
# Tool to download the validation layer libs
RUN apt update && apt install curl -y
# Get the latest package
RUN curl https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz?u= --output vulkan-sdk.tar.gz
RUN tar -xf vulkan-sdk.tar.gz
# Rename folder name
RUN mv [1-9].[0-9].[0-9]*.[0-9] vulkan-sdk

FROM debian:testing-slim
# VK packages
RUN apt update && \
	apt install gcc g++ make vulkan-tools glslang-tools exuberant-ctags \
	libgles2-mesa mesa-vulkan-drivers vulkan-validationlayers-dev libvulkan-dev \
	glslang-dev libftgl-dev libcglm-dev libglfw3-dev libstb-dev -y && \
	apt clean -y && apt autoremove -y
# Copy the binaries
COPY --from=vulkan-sdk-builder vulkan-sdk/x86_64/lib/libVkLayer_*.so /usr/local/lib/
# environment variable path to libs folder
ENV LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
# New working dir
WORKDIR /home/mainCraft/VK_App
# Volume/mount point
VOLUME ["/home/mainCraft/VK_App"]
