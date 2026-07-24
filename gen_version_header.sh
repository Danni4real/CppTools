#!/bin/bash

> ./src/version.h # clear old contents

echo "#ifndef VERSION_H" >> ./src/version.h
echo "#define VERSION_H" >> ./src/version.h
echo "" >> ./src/version.h

date +"#define VERSION \"%Y%m%d_%H%M%S\"" >> ./src/version.h

echo "" >> ./src/version.h
echo "#endif //VERSION_H" >> ./src/version.h
