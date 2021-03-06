#!/bin/bash
export QT_QPA_PLATFORM='offscreen'
QTEST_FUNCTION_TIMEOUT='800000'
cp -r ./testResource/* ${HOME}/图片/
cp -r ./testResource/* ${HOME}/Pictures/

cd ..
rm -rf ./build-ut
rm -rf ./build
rm -rf ${HOME}/.local/share/deepin/deepin-album
mkdir build-ut

cmake . -B build -D DOTEST=ON
cd build
make  -j8

lcov --directory ./CMakeFiles/deepin-album_test.dir --zerocounters
./src/deepin-album_test

lcov --directory . --capture --output-file ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ begin\ ====================\ 
lcov --remove ./coverageResult/deepin-album_Coverage.info '*/deepin-album_test_autogen/*' '*/deepin-album_autogen/*' '*/usr/include/*' '*/tests/*' '*/googletest/*' '*/UnionImage/*' -o ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ end\ ====================\ 
genhtml -o ./coverageResult/report ./coverageResult/deepin-album_Coverage.info

sleep 2

lcov --directory . --capture --output-file ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ begin\ ====================\ 
lcov --remove ./coverageResult/deepin-album_Coverage.info '*/deepin-album_test_autogen/*' '*/deepin-album_autogen/*' '*/usr/include/*' '*/tests/*' '*/googletest/*' '*/UnionImage/*' -o ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ end\ ====================\ 
genhtml -o ./coverageResult/report ./coverageResult/deepin-album_Coverage.info



cd ./../build-ut

cp -r ./../build/coverageResult/report/ ./
mv report html
cd html
mv index.html cov_deepin-album.html

cd ..
mkdir report
cd report
cp ./../../build/report/report_deepin-album.xml ./

cd ..
cp ./../build/asan_deepin-album.log* ./asan_deepin-album.log

exit 0
