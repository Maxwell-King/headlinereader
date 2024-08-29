#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <leptonica/pix.h>
#include <leptonica/pix_internal.h>
#include <windows.h>

#include <cstdlib> 
#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

void copy_to_clipboard(const char* headline);
float get_average_sym_h(Pix* line);
Pix* preproccess(Pix* img);
void cstr_replace(const char str[2000], const char* find, const char* rpwith, char ret[2000]);
/*
Gets headline from image of newspaper.
Works best on scanned images where there is more text than photo.
Receives: Filename of image, dirname of image, debug option
Returns: Headline (sometimes) (aka line(s) with biggest font size) 
*/
void get_headline(const std::string filename, const std::string dirname, const bool out)
{
	fs::path dirpath = fs::path(dirname);
	fs::current_path(dirpath);
	std::cout << "FILENAME: " << filename << "\n";
	std::cout << "DIRNAME: " << dirname << "\n";
	tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();

	std::string fncpy = filename;
	std::string basename = fncpy.erase(fncpy.find("."));
	// Initialize tesseract-ocr with English, without specifying tessdata path
	if (api->Init(NULL, "eng"))
	{
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}
	api->SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
	Pix* image_jpg = pixRead(filename.c_str());
	image_jpg = preproccess(image_jpg);
	api->SetImage(image_jpg);
	api->Recognize(0);

	BOXA* boxes = api->GetComponentImages(tesseract::RIL_TEXTLINE, false, NULL, NULL);
	if (boxes->n == 0) 
	{
		std::cout << "Failed to locate any text.\n";
		api->End();
		delete api;
		pixDestroy(&image_jpg);
		return;
	}
	PIXA* ppix = pixaCreateFromBoxa(image_jpg, boxes, 0, (boxes->n) / 4, NULL);
	char headline[2000];
	headline[0] = '\0';
	api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);

	BOX* boxfbbox = boxaGetBox(boxes, 0, L_CLONE);
	PIX* pixfppix = pixaGetPix(ppix, 0, L_CLONE);
	api->SetRectangle(boxfbbox->x, boxfbbox->y, boxfbbox->w, boxfbbox->h);
	float largesth = get_average_sym_h(pixfppix);

	memset(headline, 0, strlen(headline));
	strcpy_s(headline, sizeof headline, api->GetUTF8Text());
	std::cout << "Text: " << headline;

	pixDestroy(&pixfppix);
	boxDestroy(&boxfbbox);
	std::cout << "Finding headline..." << "\n";
	for (int i = 1; i < (boxes->n) / 4; i++)
	{
		BOX* boxfbbox = boxaGetBox(boxes, i, L_CLONE);
		PIX* pixfppix = pixaGetPix(ppix, i, L_CLONE);
		api->SetRectangle(boxfbbox->x, boxfbbox->y, boxfbbox->w, boxfbbox->h);
		float avg_h = get_average_sym_h(pixfppix);

		char* howread = api->GetUTF8Text();
		std::cout << "Text: " << howread;
		if (avg_h > largesth * 0.8)
		{
			if (avg_h < largesth * 1.2)	strcat_s(headline, sizeof headline, howread);
			else
			{
				memset(headline, 0, strlen(headline));
				strcpy_s(headline, sizeof headline, howread);
				largesth = avg_h;
			}
		}
		delete[] howread;
		pixDestroy(&pixfppix);
		boxDestroy(&boxfbbox);
	}
	std::cout << "HEADLINE: " << headline;
	copy_to_clipboard(headline);

	api->End();
	delete api;
	pixDestroy(&image_jpg);
	if (out) 
	{
		std::ofstream outfile;
		outfile.open("headlines.txt", std::ios_base::app); 
		char ret[2000];
		cstr_replace(headline, "\n", " ", ret);
		outfile << "FILENAME: " << filename << "\tHEADLINE: " << ret << "\n";
	}
}

void cstr_replace(const char str[2000], const char* find, const char* rpwith, char ret[2000])
{
	strcpy_s(ret, 2000, str);
	char* index = strpbrk(ret, find);
	if (index != NULL) *index = ' ';
	while (index != NULL)
	{
		index = strpbrk(index + 1, find);
		if (index != NULL) *index = ' ';
	}
}

void copy_to_clipboard(const char* headline)
{
	const size_t len = strlen(headline) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);

	memcpy(GlobalLock(hMem), headline, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

/*
Get average symbol height per text line.
Receives: Pix* to image of textline
Returns: Average height of symbols per textline
*/
float get_average_sym_h(Pix* line)
{
	tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
	if (api->Init(NULL, "eng"))
	{
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}
	api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	api->SetImage(line);
	api->Recognize(0);
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
	int h_sum = 0;
	int c_sum = 0;
	float avg_h = 0.0f;

	if (ri != 0) {
		do {
			const char* word = ri->GetUTF8Text(level);
			float conf = ri->Confidence(level);
			int x1, y1, x2, y2;
			ri->BoundingBox(level, &x1, &y1, &x2, &y2);
			if (conf >= 95.0f)
			{
				int height = y2 - y1;
				h_sum += height;
				c_sum++;
			}
			delete[] word;
		} while (ri->Next(level));
	}

	if (c_sum != 0) avg_h = (float)h_sum / c_sum;
	api->End();
	delete api;
	pixDestroy(&line);
	return avg_h;
}

Pix* preproccess(Pix* img)
{
	std::cout << "Preprocessing image..." << "\n";
	int hw = 2; int hh = 2;
	int sx = 10; int sy = 15; int smoothx = 2; int smoothy = 2;
	int thresh = 100; int mincount = 50; int bgval = 255; float scorefract = 0.1;

	L_KERNEL* blur = makeGaussianKernel(hw, hh, 3, 1);
	img = pixConvolveRGB(img, blur);
	img = pixConvertRGBToGray(img, 0.0, 0.0, 0.0);
	img = pixDilateGray(img, 3, 3);
	img = pixAddBorder(img, sx, bgval);
	img = pixOtsuThreshOnBackgroundNorm(img, NULL, sx, sy, thresh, mincount, bgval, smoothx, smoothy, scorefract, NULL);

	return img;
}
