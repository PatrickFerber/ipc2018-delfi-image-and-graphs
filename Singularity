Bootstrap: docker
From: ubuntu

%setup
     ## The "%setup"-part of this script is called to bootstrap an empty
     ## container. It copies the source files from the branch of your
     ## repository where this file is located into the container to the
     ## directory "/planner". Do not change this part unless you know
     ## what you are doing and you are certain that you have to do so.

    REPO_ROOT=`dirname $SINGULARITY_BUILDDEF`
    cp -r $REPO_ROOT/ $SINGULARITY_ROOTFS/planner

%post

    ## The "%post"-part of this script is called after the container has
    ## been created with the "%setup"-part above and runs "inside the
    ## container". Most importantly, it is used to install dependencies
    ## and build the planner. Add all commands that have to be executed
    ## once before the planner runs in this part of the script.

    ## Install all necessary dependencies.
    apt update
    apt -y install cmake gawk g++ g++-multilib make python python-dev python-pip
    apt -y install python3 python3-dev python3-pip
    #pip install h5py keras numpy pillow scipy tensorflow subprocess32
    pip3 install h5py keras numpy pillow scipy tensorflow

    ## Build your planner
    cd /planner
    ./build.py release64 -j4
    cd /planner/symba
    ./build -j4


%runscript
    ## The runscript is called whenever the container is used to solve
    ## an instance.

    DOMAINFILE=$1
    PROBLEMFILE=$2
    PLANFILE=$3

    ## Call your planner.
    python3 /planner/plan-ipc.py \
        $DOMAINFILE \
        $PROBLEMFILE \
        $PLANFILE

## Update the following fields with meta data about your submission.
## Please use the same field names and use only one line for each value.
%labels
Name        some-fancy-name
Description TODO
Authors     Michael Katz <michael.katz1@ibm.com> and Silvan Sievers <silvan.sievers@unibas.ch>
SupportsDerivedPredicates yes
SupportsQuantifiedPreconditions no
SupportsQuantifiedEffects no
