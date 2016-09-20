#!/bin/bash

# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

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

autorun_unit_tests=true;

if $autorun_unit_tests; then
	autorun_readback_test=false;
	autorun_msd_intel_tests=true;
	autorun_magma_app_tests=true;
	autorun_magma_sys_tests=true;
else
	autorun_readback_test=true;
	autorun_msd_intel_tests=false;
	autorun_magma_app_tests=false;
	autorun_magma_sys_tests=false;
fi

$tools_path/gn gen $build_dir --root=$fuchsia_root --dotfile=$fuchsia_root/magma/.gn --check --args="unit_test=$autorun_msd_intel_tests readback_test=$autorun_readback_test"

echo "Building magma_service_driver"
$tools_path/ninja -C $build_dir magma_service_driver magma_tests

rm -rf $bootfs_path
mkdir -p $bootfs_path/bin
cp $build_dir/msd-intel-gen $bootfs_path/bin/driver-pci-8086-1916

mkdir -p $bootfs_path/lib
cp $tools_path/sysroot/x86_64-fuchsia/lib/*libc*.so* $bootfs_path/lib
cp $tools_path/sysroot/x86_64-fuchsia/lib/*libunwind.so* $bootfs_path/lib

autorun_path=$bootfs_path/autorun

if $autorun_magma_app_tests; then
	echo "Enabling magma application driver tests to autorun"

	test_executable=bin/magma_app_unit_tests
	cp $build_dir/magma_app_unit_tests $bootfs_path/$test_executable

	echo "echo Running magma application driver unit tests" >> $autorun_path # for sanity
	echo "/boot/$test_executable" >> $autorun_path # run the tests
fi

if $autorun_magma_sys_tests; then
	echo "Enabling magma system driver tests to autorun"

	test_executable=bin/magma_sys_unit_tests
	cp $build_dir/magma_sys_unit_tests $bootfs_path/$test_executable

	echo "echo Running magma system driver unit tests" >> $autorun_path # for sanity
	echo "/boot/$test_executable" >> $autorun_path # run the tests
fi

mkdir -p $bootfs_output_dir
$tools_path/mkbootfs -v -o $bootfs_output_file @$bootfs_path

echo "Recommended bootserver command:"
echo ""
echo "$tools_path/bootserver $magenta_build_dir/magenta.bin $bootfs_output_file"
echo ""