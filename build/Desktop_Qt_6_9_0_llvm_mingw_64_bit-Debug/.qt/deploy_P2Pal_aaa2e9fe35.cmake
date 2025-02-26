include(D:/IIT/Advanced OS/New folder/P2Pal/build/Desktop_Qt_6_9_0_llvm_mingw_64_bit-Debug/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/P2Pal-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE D:/IIT/Advanced OS/New folder/P2Pal/build/Desktop_Qt_6_9_0_llvm_mingw_64_bit-Debug/P2Pal.exe
    GENERATE_QT_CONF
)
