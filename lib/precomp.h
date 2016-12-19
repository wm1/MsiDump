
#pragma once

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <setupapi.h>
#include <wtypes.h>
#include <process.h>
#include <string>
#include <fstream>
using namespace std;

#include "MsiDumpPublic.h"
#include "MsiTable.h"
#include "MsiUtils.h"

#define MSISOURCE_COMPRESSED 0x00000002

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)

extern wofstream trace;
