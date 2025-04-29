# Used to clean all make files

ret_dur=$(pwd)
# find all make files
# run make build on them
find . -name "Makefile" | while read file; do
    dir=$(dirname $(realpath $file))
    echo "Building $dir/Makefile"
    pushd . > /dev/null
    cd $dir
    make clean
    popd > /dev/null
done
cd $ret_dur