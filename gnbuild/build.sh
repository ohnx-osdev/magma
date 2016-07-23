#!/bin/bash
set -e
fuchsia_root=`pwd`
tools_path=$fuchsia_root/buildtools
magenta_build_dir=$fuchsia_root/packages/prebuilt/downloads/magenta/pc-x86-64

bootfs_output_dir=$fuchsia_root/out/Debug
build_dir=$1
if [ "$build_dir" == "" ]
    then build_dir=$bootfs_output_dir
fi
bootfs_path=$build_dir/bootfs

bootfs_output_file=$bootfs_output_dir/user.bootfs
rm -f $bootfs_output_file

echo "fuchsia_root=$fuchsia_root build_dir=$build_dir"

$tools_path/gn gen $build_dir --root=$fuchsia_root --dotfile=$fuchsia_root/magma/.gn --check

echo "Building magma_service_driver"
$tools_path/ninja -C $build_dir magma_service_driver

rm -rf $bootfs_path
mkdir -p $bootfs_path/bin
cp $build_dir/msd-intel-gen $bootfs_path/bin/driver-pci-8086-1616

if true; then
	echo "Building magma_tests"
	$tools_path/ninja -C $build_dir magma_tests

	test_executable=bin/magma_unit_tests
	cp $build_dir/magma_unit_tests $bootfs_path/$test_executable

	autorun_path=$bootfs_path/autorun
	echo "echo \"Running Magma Unit Tests\"" >> $autorun_path # for sanity
	echo "/boot/$test_executable" >> $autorun_path # run the tests
	echo "msleep 1000" >> $autorun_path # give some time to write out to log listener
	echo "\`poweroff" >> $autorun_path # rinse and repeat
fi


mkdir -p $bootfs_output_dir
$tools_path/mkbootfs -v -o $bootfs_output_file @$bootfs_path

echo "Recommended bootserver command:"
echo ""
echo "$tools_path/bootserver $magenta_build_dir/magenta.bin $bootfs_output_file"
echo ""