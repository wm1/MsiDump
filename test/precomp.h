
#pragma once

#include <Windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

#include "MsiDumpPublic.h"

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)
