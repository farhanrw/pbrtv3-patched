# CMake generated Testfile for 
# Source directory: /home/farhan/Farhan_Thesis_Codes/pbrt-v3
# Build directory: /home/farhan/Farhan_Thesis_Codes/pbrt-v3/cmake-build-release
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(pbrt_unit_test "pbrt_test")
set_tests_properties(pbrt_unit_test PROPERTIES  _BACKTRACE_TRIPLES "/home/farhan/Farhan_Thesis_Codes/pbrt-v3/CMakeLists.txt;559;ADD_TEST;/home/farhan/Farhan_Thesis_Codes/pbrt-v3/CMakeLists.txt;0;")
subdirs("src/ext/openexr")
subdirs("src/ext/glog")
subdirs("src/ext/ptex")