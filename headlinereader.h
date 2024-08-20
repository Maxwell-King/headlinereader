#pragma once
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <leptonica/pix.h>
#include <leptonica/pix_internal.h>
#include <windows.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>

void get_headline(const std::string filename, const std::string dirname, const bool out);
