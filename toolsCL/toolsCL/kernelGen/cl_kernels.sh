#! /bin/bash
# This script converts all OpenCL Kernels to C++ char strings
# Outputs (overwrites): cl_kernels.hpp and cl_kernels.cpp

CL_HEADERDIR="./cl_headers/*.cl"
CL_KERNELDIR="./cl_kernels/*.cl"
HEADER='../cl_kernels.hpp'
INCHEADER='cl_kernels.hpp'
SOURCE='../cl_kernels.cpp'

echo "// AUTOMATICALLY GENERATED FILE, DO NOT EDIT" > $HEADER
echo "// AUTOMATICALLY GENERATED FILE, DO NOT EDIT" > $SOURCE

echo "#ifndef CL_KERNELS_HPP_" >> $HEADER
echo "#define CL_KERNELS_HPP_" >> $HEADER
echo "#include <iostream>" >> $HEADER
echo "#include <string>" >> $HEADER

echo "#include \"$INCHEADER\"" >> $SOURCE
echo "#include <sstream>" >> $SOURCE
echo "#include <string>" >> $SOURCE

echo "void RegisterKernels(std::string &strSource);" >> $HEADER


shopt -s nullglob
for CL_KERNEL in $CL_HEADERDIR $CL_KERNELDIR
do
	CL_KERNEL_STR=`cat $CL_KERNEL`
	CL_KERNEL_NAME=`echo $CL_KERNEL`
	CL_KERNEL_NAME="${CL_KERNEL_NAME##*/}"
	CL_KERNEL_NAME="${CL_KERNEL_NAME%.cl}"
	echo -n "std::string ${CL_KERNEL_NAME} = \"" >> $SOURCE
	echo -n "$CL_KERNEL_STR" | sed -e ':a;N;$!ba;s/\n/\\n/g' | sed -e 's/\"/\\"/g' >> $SOURCE
	echo "\";  // NOLINT" >> $SOURCE
done



echo "void RegisterKernels(std::string &strSource) {" >> $SOURCE
echo "  std::stringstream ss;" >> $SOURCE

shopt -s nullglob
for CL_KERNEL in $CL_HEADERDIR
do
	CL_KERNEL_NAME=`echo $CL_KERNEL`
	CL_KERNEL_NAME="${CL_KERNEL_NAME##*/}"
	CL_KERNEL_NAME="${CL_KERNEL_NAME%.cl}"
	echo "  ss << $CL_KERNEL_NAME << \"\\n\\n\";  // NOLINT" >> $SOURCE
done

shopt -s nullglob
for CL_KERNEL in $CL_KERNELDIR
do
	CL_KERNEL_NAME=`echo $CL_KERNEL`
	CL_KERNEL_NAME="${CL_KERNEL_NAME##*/}"
	CL_KERNEL_NAME="${CL_KERNEL_NAME%.cl}"
	echo "  ss << ${CL_KERNEL_NAME} << \"\\n\\n\";  // NOLINT" >> $SOURCE
done


echo "  strSource = ss.str();" >> $SOURCE

echo "}" >> $SOURCE

echo "#endif//#ifndef CL_KERNELS_HPP_" >> $HEADER