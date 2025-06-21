mkdir Build_excelprocess
cd Build_excelprocess

mkdir build64_debug
cd build64_debug
rm * -rf

cmake -Wno-dev -DCMAKE_BUILD_TYPE=DynamicRelease  -DCMAKE_BUILD_TOOLS:BOOL=True ../..
make -j16
