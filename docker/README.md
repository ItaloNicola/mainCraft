# mainCraft Docker

This is a tiny documentation about the mainCraft Docker and related things.

## Dockerfile

This Dockerfile contains the minimun necessary to compile and run the mainCraft.
To generate a container image use this command:

``` bash
sudo docker build --tag maincraft:1.0 .
```

### Image leftover

After this you may want to delete the leftover image that this Dockerfile
generates. First you need to find the ID(s) using `sudo docker images`:

``` bash
sudo docker images
REPOSITORY            TAG                     IMAGE ID            CREATED             SIZE
maincraft             1.0                     49be0182fd0c        2 minutes ago       851MB
<none>                <none>                  e03282a65971        5 minutes ago       2.16GB
<none>                <none>                  0fa060a4a456        12 minutes ago      105MB
```

In my case the ID(s) are `e03282a65971` and `0fa060a4a456`. To remove use this command:

``` bash
sudo docker rmi <ID 1> <ID 2>.
```

### Docker Fail to generate

Sometimes the curl command fails and the docker gets stucked. To solve this issue
you need to use `--no-cache` flag.

## Dock.sh

This is script aims to help users and developers to compile and run the mainCraft
insider a docker container.
To use it you need pass the name of container image and the command that you want
execute inside the container, as demonsted in this example:

``` bash
./dock.sh -c mainCraft:1.0 -i 'make all'
```

