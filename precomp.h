
#pragma once

#include <windows.h>
#include <wtypes.h>
#include <process.h>
#include <string>
#include <fstream>
using namespace std;

#include <propidlbase.h>
#include <msiquery.h>
#include <msidefs.h>
#include <setupapi.h>

#include "MsiDumpPublic.h"
#include "MsiTable.h"
#include "MsiUtils.h"

#define MSISOURCE_COMPRESSED 0x00000002

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)
#define SET_FLAG(field, flag) ((field) |= (flag))
#define CLEAR_FLAG(field, flag) ((field) &= ~(flag))

extern wofstream trace;
