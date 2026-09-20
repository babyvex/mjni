#include <iostream>

#include "mJNI/mJNI.h"

int main() {

    mJNI::mJNIAttachToJVM(mJNI::Utils::getPidFromProcessName("javaw.exe"));

    auto klass = mJNI::findClass("net/minecraft/class_310");

    std::printf("klass %p \n", klass);

    system("pause");

    return 0;

}