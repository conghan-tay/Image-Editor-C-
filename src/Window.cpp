#include "precompiled.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>

#include<stdio.h>
#include<stdlib.h>
#include <numeric>
#include <complex>
#include <ctime>
#include <random>
#include <chrono>

using c64 = std::complex<double>;
// `cooley_tukey` does the cooley-tukey algorithm, recursively
template <typename Iter>
void cooley_tukey(Iter first, Iter last) {
	auto size = last - first;
	if (size >= 2) {
		// split the range, with even indices going in the first half,
		// and odd indices going in the last half.
		/*auto temp = std::vector<c64>(size / 2);
		for (size_t i = 0; i < size / 2; ++i) {
			temp[i] = first[i * 2 + 1];
			first[i] = first[i * 2];
		}
		for (size_t i = 0; i < size / 2; ++i) {
			first[i + size / 2] = temp[i];
		}*/

		// recurse the splits and butterflies in each half of the range
		auto split = first + size / 2;
		cooley_tukey(first, split);
		cooley_tukey(split, last);

		// now combine each of those halves with the butterflies
		for (size_t k = 0; k < size / 2; ++k) {
			//auto w = std::exp(c64(0, -2.0 * (double)M_PI * k / size));

			auto& bottom = first[k];
			auto& top = first[k + size / 2];
			/*top = bottom - w * top;
			bottom -= top - bottom;*/

			/*std::complex<double> t = std::polar(1.0, -2 * M_PI * k / N) * odd[k];
			x[k] = even[k] + t;
			x[k + N / 2] = even[k] - t;
*/
			std::complex<double> t = std::polar(1.0, -2 * M_PI * k / size) * top;
			auto temp = bottom;
			bottom = temp + t;
			top = temp - t;

		}
	}
}

/******************************************
START
Code to read P6 PPM,
Ignore if not required
*******************************************/
typedef struct {
	unsigned char red, green, blue;
} PPMPixel;

typedef struct {
	int x, y;
	PPMPixel *data;
} PPMImage;

#define CREATOR "Conghan"
#define RGB_COMPONENT_COLOR 255

void writePPM(const char *filename, PPMImage *img)
{
	FILE *fp;
	//open file for output
	fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//write the header file
	//image format
	fprintf(fp, "P6\n");

	//comments
	fprintf(fp, "# Created by %s\n", CREATOR);

	//image size
	fprintf(fp, "%d %d\n", img->x, img->y);

	// rgb component depth
	fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);

	// pixel data
	fwrite(img->data, 3 * img->x, img->y, fp);
	fclose(fp);
}
/******************************************
END
Code to read P6 PPM,
Ignore if not required
*******************************************/

namespace
{
	const int histogramHeight = 100;

	const int textbarHeight = 20;
	int statusBarWidths[2] = { 128, 0 };
} // anon namespace

namespace cs370
{
	Window::Window(const wxString& title, const wxPoint& pos, const wxSize& size) :
		wxFrame(nullptr, -1, title, pos, size), cmdBox_(nullptr), imgMenu_(nullptr), pwd_(wxGetCwd()),
		m_imageWidth(0), m_imageHeight(0), sbmp(nullptr), c_image(nullptr), m_imageRGB(nullptr), m_imageBitmap(nullptr),
		m_interP(INTERPOLANT::NN), new_image(nullptr), m_colorRange(255), unsharp_gaussian(nullptr), be4gauss_image(nullptr),
		be4_equalized(nullptr)
	{
		/*
		* Set status bar
		*/
		CreateStatusBar(2);
		::statusBarWidths[1] = this->GetClientSize().GetWidth() - 128;
		SetStatusWidths(2, ::statusBarWidths);
		SetStatusText(pwd_, 1);

		/*
		* Set menu bar
		*/
		filemenu = new wxMenu;
		filemenu->Append(EV_M_FILE_OPEN, wxT("Open..."));
		filemenu->Append(EV_M_FILE_SAVE, wxT("Save"));
		filemenu->Append(EV_M_FILE_SAVEAS, wxT("Save As..."));
		filemenu->AppendSeparator();
		filemenu->Append(EV_M_FILE_EXIT, wxT("Exit"));

		imgMenu_ = new wxMenu;
		imgMenu_->Append(EV_M_IMG_NN, wxT("Nearest Neighbour Interpolation"), wxT("Use nearest neighbour interpolation"), true);
		imgMenu_->Check(EV_M_IMG_NN, true);
		imgMenu_->Append(EV_M_IMG_BILINEAR, wxT("Bilinear Interpolation"), wxT("Use bilinear interpolation"), true);
		imgMenu_->AppendSeparator();

		imgMenu_->Append(EV_M_IMG_ADD, wxT("Add"));
		imgMenu_->Append(EV_M_IMG_SUBSTRACT, wxT("Subtract"));
		imgMenu_->Append(EV_M_IMG_PRODUCT, wxT("Product"));
		imgMenu_->Append(EV_M_IMG_NEGATIVE, wxT("Image Negative"));
		imgMenu_->Append(EV_M_IMG_LOGTRANSFORM, wxT("Log Transform"));
		imgMenu_->Append(EV_M_IMG_POWERTRANSFORM, wxT("Power Transform"));
		imgMenu_->Append(EV_M_IMG_GAUSSIAN, wxT("Gaussian Filter"));
		imgMenu_->Append(EV_M_IMG_SOBEL, wxT("Sobel Filter"));
		imgMenu_->Append(EV_M_IMG_UNSHARP, wxT("Unsharp Mask"));
		imgMenu_->Append(EV_M_CONNECTED_COMPONENT, wxT("M Connected Labelling"));
		imgMenu_->Append(EV_M_VIEWHISTOGRAMS, wxT("Histograms"));
		imgMenu_->Append(EV_M_EQUALIZE, wxT("Equalise Histograms"));
		imgMenu_->Append(EV_M_FT_DIRECT, wxT("Display Fourier Spectrum Using Direct"));
		imgMenu_->Append(EV_M_FT_SEP, wxT("Display Fourier Spectrum Using Separable Direct"));
		imgMenu_->Append(EV_M_FT_FFT, wxT("Fast Fourier Transform"));
		imgMenu_->Append(EV_M_GAUSSIAN_NOISE, wxT("Gaussian Noise"));
		imgMenu_->Append(EV_M_SALT_PEPPER, wxT("Salt and Pepper"));
		imgMenu_->Append(EV_M_MEDIAN, wxT("3x3 Median"));
		imgMenu_->Append(EV_M_LOCAL_NOISE, wxT("Local Noise Reduction"));
		imgMenu_->Append(EV_M_ADAPTIVE, wxT("Adaptive Median Noise"));
		imgMenu_->Append(EV_M_MEAN, wxT("Mean filter"));

		imgMenu_->AppendSeparator();
		imgMenu_->Append(EV_M_IMG_REVERT, wxT("Revert image to original"));
		imgMenu_->Append(EV_M_IMG_PROPERTIES, wxT("Properties"));

		helpmenu = new wxMenu;
		helpmenu->Append(EV_M_HELP_HELP, wxT("Help"));
		helpmenu->Append(EV_M_HELP_ABOUT, wxT("About"));

		menubar = new wxMenuBar;
		menubar->Append(filemenu, wxT("File"));
		menubar->Append(imgMenu_, wxT("Image"));
		menubar->Append(helpmenu, wxT("Help"));

		SetMenuBar(menubar);

		// Set window dimensions
		const wxSize clientsize(this->GetClientSize());
		// Set textbox control
		/*cmdBox_ = new wxTextCtrl(
			this,
			EV_TEXTBOX,
			wxT(""),
			wxPoint(0, 0),
			wxSize(clientsize.GetWidth(), ::textbarHeight),
			wxTE_PROCESS_ENTER);
		cmdBox_->SetFocus();*/


	}

	Window::~Window()
	{

		if (m_imageRGB != nullptr)
			delete m_imageRGB;
		if (m_imageBitmap != nullptr)
			delete m_imageBitmap;
		if (sbmp != nullptr)
			delete sbmp;

		if (c_image != nullptr)
			delete[] c_image;

		if (unsharp_gaussian != nullptr)
			delete[] unsharp_gaussian;

		if (be4gauss_image != nullptr)
			delete[] be4gauss_image;

		if (be4_equalized != nullptr)
			delete[] be4_equalized;
		/*if (new_image)
		delete[] new_image;*/
		/*delete filemenu;
		delete imgMenu_;
		delete helpmenu;*/

	}

	wxSize Window::GetImgPanelSize() const
	{
		return wxSize(this->GetClientSize().GetWidth(), this->GetClientSize().GetHeight());
	}

	void Window::SetImgPanelSize(const wxSize& size)
	{
		SetImgPanelSize(size.GetWidth(), size.GetHeight());
	}

	void Window::ResizeImage(unsigned char *& ori_image, unsigned char *& target_image, int c_width, int c_height, int w, int h, int new_totalSize)
	{
		/*if (w != c_width || h != c_height)
		{*/

			if (m_interP == INTERPOLANT::NN)
			{
				target_image = new unsigned char[new_totalSize];

				for (unsigned i = 0; i < (unsigned)w; ++i)
				{
					for (unsigned j = 0; j < (unsigned)h; ++j)
					{
						unsigned x = 0, y = 0;
						x = (float)i / ((float)w / c_width);
						y = (float)j / ((float)h / c_height);
						target_image[(i * 3) + (j * (w * 3))] = ori_image[(x * 3) + (y * (c_width * 3))];
						target_image[(i * 3) + (j * (w * 3)) + 1] = ori_image[(x * 3) + (y * (c_width * 3)) + 1];
						target_image[(i * 3) + (j * (w * 3)) + 2] = ori_image[(x * 3) + (y * (c_width * 3)) + 2];
					}
				}

				//new_staticbmp(target_image, w, h);


			}
			else
			{
				target_image = new unsigned char[new_totalSize];

				for (unsigned i = 0; i < (unsigned)w; ++i)
				{
					for (unsigned j = 0; j <(unsigned)h; ++j)
					{
						unsigned x = 0, y = 0;
						float x_diff = 0.f, y_diff = 0.f;
						x = (float)i / ((float)w / c_width);
						y = (float)j / ((float)h / c_height);
						x_diff = (float)i / ((float)w / c_width) - x;
						y_diff = (float)j / ((float)h / c_height) - y;

						int i1, i2, i3;
						i1 = (x * 3) + (y * (c_width * 3));
						i2 = (x * 3) + (y * (c_width * 3)) + 1;
						i3 = (x * 3) + (y * (c_width * 3)) + 2;

						float c0, c1,
							c2, c3;

						float m = 0.f;
						float m1 = 0.f;
						float m2 = 0.f;
						if (x < (unsigned)c_width - 1 && y < (unsigned)c_height - 1)
						{
							c0 = (float)ori_image[i1], c1 = (float)ori_image[i1 + 3],
								c2 = (float)ori_image[i1 + (c_width * 3)], c3 = (float)ori_image[i1 + (c_width * 3) + 3];

							float l = c0 + y_diff * (c2 - c0);
							float r = c1 + y_diff * (c3 - c1);

							m = l + (r - l) * x_diff;

							c0 = (float)ori_image[i2], c1 = (float)ori_image[i2 + 3],
								c2 = (float)ori_image[i2 + (c_width * 3)], c3 = (float)ori_image[i2 + (c_width * 3) + 3];

							l = c0 + y_diff * (c2 - c0);
							r = c1 + y_diff * (c3 - c1);

							m1 = l + (r - l) * x_diff;

							c0 = (float)ori_image[i3], c1 = (float)ori_image[i3 + 3],
								c2 = (float)ori_image[i3 + (c_width * 3)], c3 = (float)ori_image[i3 + (c_width * 3) + 3];

							l = c0 + y_diff * (c2 - c0);
							r = c1 + y_diff * (c3 - c1);

							m2 = l + (r - l) * x_diff;
						}
						else
						{
							m = (float)ori_image[i1];
							m1 = (float)ori_image[i1];
							m2 = (float)ori_image[i1];
						}

						target_image[(i * 3) + (j * (w * 3))] = (unsigned char)m;
						target_image[(i * 3) + (j * (w * 3)) + 1] = (unsigned char)m1;
						target_image[(i * 3) + (j * (w * 3)) + 2] = (unsigned char)m2;
					}
				}

			}
		//}
	}

	void Window::SetImgPanelSize(int w, int h)
	{
		this->SetClientSize(w, h );

		if (cmdBox_ != nullptr)
		{
			cmdBox_->SetSize(w, ::textbarHeight);
			cmdBox_->SetPosition(wxPoint(0, 0));
		}

		if (c_image != nullptr)
		{
			new_w = w;
			new_h = h;
			new_size = w * h * 3;

			/*if (new_image != nullptr)
				delete[] new_image;*/

			ResizeImage(c_image, new_image, m_imageWidth, m_imageHeight, w, h, new_size);

			new_staticbmp(new_image, w, h);

		} // end of c_image != nullptr

	}




	void Window::OnResize(wxSizeEvent &)
	{
		SetImgPanelSize(GetImgPanelSize());
	}


	void Window::LoadImageFromFile(wxString & file_name, unsigned char * &image, int &width, int &height, int &totalSize)
	{
		//image_load = new wxImage(fileName, wxBITMAP_TYPE_ANY, -1);
		std::ifstream infile{ std::string(file_name) };
		std::string c_line;

		// color range hard code to 255
		// for ppmt just set to PPMTYPE::P3
		// calculate width and height
		// totalSize
		// check if image != nullptr, delete
		// copy over values from wxImage

		wxImage * load_image = new wxImage(file_name, wxBITMAP_TYPE_ANY, -1);
		m_colorRange = 255;
		ppmt = PPMTYPE::P3;
		width = load_image->GetWidth();
		height = load_image->GetHeight();
		totalSize = width * height * 3;
		if (image != nullptr)
			delete[] image;
		image = new unsigned char[totalSize];

		std::copy(load_image->GetData(), load_image->GetData() + (totalSize), image);
		delete load_image;

	}

	void Window::OnFileOpen(wxCommandEvent &)
	{
		wxFileDialog
			openFileDialog(this, _("Open XYZ file"), "", "",
				"*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_OK)
		{
			CurrentDocPath = openFileDialog.GetPath();
			// Sets our current document to the file the user selected
			//MainEditBox->LoadFile(CurrentDocPath); //Opens that file
			SetTitle(wxString("Edit - ") <<
				openFileDialog.GetFilename()); // Set the Title to reflect the file open

			LoadImageFromFile(CurrentDocPath, c_image, m_imageWidth, m_imageHeight, m_totalSize);

			equalized = false;
		}

		SetImgPanelSize(GetImgPanelSize());


	}

	void Window::OnFileSave(wxCommandEvent &)
	{
		std::string path(CurrentDocPath);

		if (ppmt == PPMTYPE::P3)
		{
			std::ofstream ofs;
			ofs.open(path, std::ofstream::out);

			if (ofs.is_open())
			{

				ofs << "P3";
				ofs << "\n";
				ofs << "\n";
				ofs << new_w << " " << new_h << "\n";
				ofs << 255 << "\n";
				int counter = 0;
				for (int i = 0; i < new_size; ++i)
				{
					ofs << (int)new_image[i];
					counter++;
					if (counter == new_w * 3)
					{
						ofs << std::endl;
						counter = 0;
					}
					else
						ofs << " ";
				}

			}

			ofs.close();
		}
		else
		{

			PPMImage * img = (PPMImage *)malloc(sizeof(PPMImage));;
			img->x = new_w;
			img->y = new_h;

			img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));/* new PPMPixel[new_w * new_h];*/
			unsigned count = 0;
			for (int i = 0; i < new_size; i += 3)
			{
				img->data[count].red = (unsigned char)new_image[i];
				img->data[count].green = (unsigned char)new_image[i + 1];
				img->data[count].blue = (unsigned char)new_image[i + 2];
				count++;
			}

			writePPM(path.c_str(), img);

			delete[] img->data;
			delete[] img;
		}



	}

	void Window::OnFileSaveAs(wxCommandEvent &)
	{
		wxFileDialog
			saveFileDialog(this, _("Save XYZ file"), "", "",
				"*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;
		wxString save_loc = saveFileDialog.GetPath();


		std::string path(save_loc);

		if (ppmt == PPMTYPE::P3)
		{
			std::ofstream ofs;
			ofs.open(path, std::ofstream::out);

			if (ofs.is_open())
			{

				ofs << "P3";
				ofs << "\n";
				ofs << "\n";
				ofs << new_w << " " << new_h << "\n";
				ofs << 255 << "\n";
				int counter = 0;
				for (int i = 0; i < new_size; ++i)
				{
					ofs << (int)new_image[i];
					counter++;
					if (counter == new_w * 3)
					{
						ofs << std::endl;
						counter = 0;
					}
					else
						ofs << " ";
				}

			}

			ofs.close();
		}
		else
		{

			PPMImage * img = (PPMImage *)malloc(sizeof(PPMImage));;
			img->x = new_w;
			img->y = new_h;

			img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));/* new PPMPixel[new_w * new_h];*/
			unsigned count = 0;
			for (int i = 0; i < new_size; i += 3)
			{
				img->data[count].red = (unsigned char)new_image[i];
				img->data[count].green = (unsigned char)new_image[i + 1];
				img->data[count].blue = (unsigned char)new_image[i + 2];
				count++;
			}

			writePPM(path.c_str(), img);

			free(img->data);
			free(img);
		}

	}

	void Window::OnExit(wxCommandEvent&)
	{
		Close(true);
	}

	void Window::OnImgNN(wxCommandEvent &)
	{
		//imgMenu_->Check(EV_M_IMG_NN, true);
		if (imgMenu_->IsChecked(EV_M_IMG_NN))
		{
			m_interP = INTERPOLANT::NN;
			imgMenu_->Check(EV_M_IMG_NN, true);
			imgMenu_->Check(EV_M_IMG_BILINEAR, false);

		}
		else
		{
			m_interP = INTERPOLANT::BL;
			imgMenu_->Check(EV_M_IMG_NN, false);
			imgMenu_->Check(EV_M_IMG_BILINEAR, true);
		}

		SetImgPanelSize(GetImgPanelSize());
	}

	void Window::OnImgBilinear(wxCommandEvent &)
	{
		if (imgMenu_->IsChecked(EV_M_IMG_BILINEAR))
		{
			m_interP = INTERPOLANT::BL;
			imgMenu_->Check(EV_M_IMG_BILINEAR, true);
			imgMenu_->Check(EV_M_IMG_NN, false);

		}
		else
		{
			m_interP = INTERPOLANT::NN;
			imgMenu_->Check(EV_M_IMG_BILINEAR, false);
			imgMenu_->Check(EV_M_IMG_NN, true);
		}
		SetImgPanelSize(GetImgPanelSize());
	}


	void Window::OnAbout(wxCommandEvent&)
	{
		wxMessageBox(
			wxT("CS370 Framework\nCopyright (C) 2014 DigiPen Institute of Technology"),
			wxT("About CS370 Framework"),
			wxOK | wxICON_INFORMATION,
			this);

		//wxEVT_COMMAND_MENU_SELECTED
	}



	void Window::AddSubtractProduct(wxCommandEvent & ev)
	{
		// Let user choose image
		// Resize chosen image
		// to currently loaded image
		// add pixels of image chosen to current image = edited_image
		// replace current image with the  edited_image

		wxFileDialog
			openFileDialog(this, _("Open file to add"), "", "",
				"*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		wxString AddDocPath;
		unsigned char * add_image = nullptr;
		int imageWidth, imageHeight, totalSize;
		unsigned char * rescaled_add = nullptr;
		unsigned char * added_image = nullptr;

		if (openFileDialog.ShowModal() == wxID_OK)
		{
			AddDocPath = openFileDialog.GetPath();

			LoadImageFromFile(AddDocPath, add_image, imageWidth, imageHeight, totalSize);

			wxSize new_dimensions = GetImgPanelSize();

			
			int full_dimensions = new_dimensions.GetWidth() * new_dimensions.GetHeight() * 3;

			//ResizeImage(add_image, rescaled_add, imageWidth, imageHeight, window_width, window_height, full_dimensions);

			// Resize to original image
			// Do arithmetic with original
			// Run SetImgPanelSize(GetImgPanelSize()); so new_image = windows size c_image

			ResizeImage(add_image, rescaled_add, imageWidth, imageHeight, m_imageWidth, m_imageHeight, m_totalSize);

			added_image = new unsigned char[full_dimensions];

			for (int i = 0; i < m_totalSize; ++i)
			{
				int value;
				if (ev.GetId() == EV_M_IMG_ADD)
					value = (int)c_image[i] + (int)rescaled_add[i];
				else if (ev.GetId() == EV_M_IMG_SUBSTRACT)
					value = (int)c_image[i] - (int)rescaled_add[i];
				else
				{
					float i_value = (float)c_image[i] / (float)m_colorRange;
					float r_value = (float)rescaled_add[i] / (float)m_colorRange;
					float o_value = i_value * r_value;
					value = (int)std::roundf(o_value * m_colorRange);//(int)new_image[i] * (int)rescaled_add[i];
				}
				value = std::min(std::max(0, value), (int)m_colorRange);
				added_image[i] = value;
			}



			ReplaceOriginal(added_image);


			delete[] add_image;
			delete[] rescaled_add;
			//delete[] added_image;



		}
	}

	void Window::ReplaceOriginal(unsigned char * replacement)
	{
		/*delete[] c_image;
		c_image = new unsigned char[m_totalSize];*/
		std::copy(replacement, replacement + m_totalSize, c_image);
		SetImgPanelSize(GetImgPanelSize());
		delete[] replacement;
	}

	void Window::Negative(wxCommandEvent &)
	{
		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];
		for (int i = 0; i < m_totalSize; ++i)
		{
			added_image[i] = m_colorRange - c_image[i];
		}

		ReplaceOriginal(added_image);

	}

	void Window::LogTransform(wxCommandEvent &)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Constant for log transform"));
		std::string text = inputtext;

		if (text == "")
			return;

		std::stringstream text_s(text);

		float c = 0.f;
		text_s >> c;

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];
		for (int i = 0; i < m_totalSize; ++i)
		{
			added_image[i] = (unsigned char)(log10((float)c_image[i] + 0.10f) * c);
		}

		ReplaceOriginal(added_image);
	}

	void Window::PowerTransform(wxCommandEvent &)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Constant and Gamma Separated by Spaces"));
		std::string text = inputtext;

		if (text == "")
			return;

		std::stringstream text_s(text);

		float c = 0.f;
		float gamma = 0.f;
		float epsilon = 0.1f;
		text_s >> c;
		text_s >> gamma;

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];
		for (int i = 0; i < m_totalSize; ++i)
		{
			added_image[i] = (unsigned char)std::min(std::max(c *  std::powf((float)c_image[i] + epsilon, gamma), 0.f), 255.0f);
		}

		ReplaceOriginal(added_image);
	}

	void Window::GaussianFilter(wxCommandEvent & )
	{
		wxString inputtext = wxGetTextFromUser(wxT("Input Kernel Size followed by SD Separated by Spaces"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		int K_size = 0;
		float SD = 0.f;
		float sum = 0.f;
		text_s >> K_size;
		text_s >> SD;
		int mean = K_size / 2;


		std::vector<float> Kernel;

		for (int r = 0; r < K_size; ++r)
		{
			//Kernel.push_back(std::vector<float>(K_size));
			for (int c = 0; c < K_size; ++c)
			{
				Kernel.push_back(exp(-0.5f * (pow((r - mean) / SD, 2.0) + pow((c - mean) / SD, 2.0)))
				/*/ (2 * M_PI * SD * SD)*/);
				sum += Kernel.back();
			}
		}

		// Normalize the kernel
		int K_size2 = K_size * K_size;
		for (int i = 0; i < K_size2; ++i)
		{
			Kernel[i] /= sum;
		}



		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		if (unsharp_gaussian != nullptr)
			delete[] unsharp_gaussian;
		unsharp_gaussian = new unsigned char[m_totalSize];

		if (be4gauss_image != nullptr)
			delete[] be4gauss_image;
		be4gauss_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				//int center_pixel = (x * 3) + (y * (new_w * 3));
				std::vector<float> pixel_values;//one color channel only

				int startX = x - mean;
				int startY = y - mean;
				int endX = x + mean;
				int endY = y + mean;

				// Get values from image for one pixel
				for (startY = y - mean; startY <= endY; ++startY)
				{
					for (startX = x - mean; startX <= endX; ++startX)
					{
						if (startX < 0 || startX >= m_imageWidth || startY <0 || startY >= m_imageHeight)
						{
							pixel_values.push_back(0.f);
						}
						else {
							int c_pixel = (startX * 3) + (startY * (m_imageWidth * 3));
							pixel_values.push_back((float)c_image[c_pixel]);
						}
					}
				}

				float new_pixel_value = std::inner_product(pixel_values.begin(), pixel_values.end(),
					Kernel.begin(), 0.f);

				added_image[(x * 3) + (y * (m_imageWidth * 3))] = (unsigned char)new_pixel_value;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = (unsigned char)new_pixel_value;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = (unsigned char)new_pixel_value;

			}
		}

		for (int i = 0; i < m_totalSize; ++i)
		{
			unsharp_gaussian[i] = /*c_image[i] -*/ added_image[i];
		}

		for (int i = 0; i < m_totalSize; ++i)
		{
			be4gauss_image[i] = c_image[i];
		}

		ReplaceOriginal(added_image);

	}

	void Window::SobelFilter(wxCommandEvent & )
	{
		float kernelx[3][3] = { { -1, 0, 1 },
		{ -2, 0, 2 },
		{ -1, 0, 1 } };

		float kernely[3][3] = { { -1, -2, -1 },
		{ 0,  0,  0 },
		{ 1,  2,  1 } };

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				float magX = 0.f;
				float magY = 0.f;

				for (int a = 0; a < 3; ++a)
				{
					for (int b = 0; b < 3; ++b)
					{
						int xn = x + a - 1;
						int yn = y + b - 1;

						int index = (xn * 3) + (yn * (m_imageWidth * 3));

						if (xn< 0 || xn >= m_imageWidth || yn<0 || yn >= m_imageHeight)
						{
						}
						else
						{
							magX += c_image[index] * kernelx[a][b];
							magY += c_image[index] * kernely[a][b];
						}
					}
				}

				float mag = std::sqrtf(powf(magX, 2.0f) + powf(magY, 2.0f));
				added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
			}
		}

		ReplaceOriginal(added_image);

	}

	void Window::UnsharpMask(wxCommandEvent &)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Input K value"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		float K = 0;
		text_s >> K;

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (int i = 0; i < m_totalSize; ++i)
		{
			float v = std::min(255.0f, std::max(0.f, (float)be4gauss_image[i] + K * (be4gauss_image[i] - unsharp_gaussian[i])));
			added_image[i] = (unsigned char)v;
		}
		ReplaceOriginal(added_image);
	}

	// map<coords, label> All_Nodes
	// map<label, Node<label> Roots
	// Get Neighboring Nodes
	// if Neighbor not empty
	// Get Smallest label from neighbors
	// All_nodes[this coord] = smallest label
	// for all other labels, union with Root[smallest label]
	// else if Neighbor empty
	// All_nodes[this coord] = label_count
	// Root[label_count] = std::unique_pointer<Node<label> (label_count)
	// ++label_count
	// map<label,std::vector<unsigned char> Label_Colors
	// for i = 0 to label_count
	// color[i] = new color
	// Loop thru xs and ys
	//All_Nodes.find != end
	// Get Label from All_Nodes
	// Use Label to get the root,
	// root to get the parent
	// get the label from parent
	// Get color using the label
#include <memory>

	void Window::MConnectedLabelling(wxCommandEvent & )
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Min and Max Values(0-255), seperated by spaces"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		unsigned int min = 0, max = 0;
		text_s >> min;
		text_s >> max;

		std::map<std::pair<unsigned, unsigned>, label> All_Nodes;
		std::map<label, std::vector<unsigned char>> Label_Colors;

		std::map<label, std::unique_ptr<Node<label>>> Roots;

		label label_count = 0;

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (int y = 0; y < m_imageHeight; ++y)
		{
			for (int x = 0; x < m_imageWidth; ++x)
			{
				unsigned char value = c_image[(x * 3) + (y * (m_imageWidth * 3))];
				if (value >= (unsigned char)min && value <= (unsigned char)max)
				{
					// Get Neighboring Nodes with values >=min and <=max using M Connected
					std::vector<std::pair<unsigned, unsigned>> Neighbors = GetNeighbors(x, y, (unsigned char)min, (unsigned char)max);
					// if Neighbor not empty
					// Get Smallest label from neighbors
					// Assign smallest label to self and neighbors
					// add self to All_Nodes
					// else if Neighbor empty
					// Assign new label to self
					// Add self to All_nodes

					// Get Smallest label from neighbors
					if (!Neighbors.empty())
					{
						label min_label = 0;
						for (auto iter = Neighbors.begin(); iter != Neighbors.end(); ++iter)
						{
							std::pair<unsigned, unsigned> coords = *iter;
							label curr = All_Nodes[coords];
							if (iter == Neighbors.begin())
							{
								min_label = curr;
							}
							else
							{

								if (curr <= min_label)
									min_label = curr;
							}
						}
						// Assign smallest label to self and neighbors
						// add self to All_Nodes
						All_Nodes[std::pair<unsigned, unsigned>(x, y)] = min_label;
						for (auto iter = Neighbors.begin(); iter != Neighbors.end(); ++iter)
						{
							std::pair<unsigned, unsigned> coords = *iter;
							Roots[All_Nodes[coords]]->Union(Roots[min_label].get());
						}

					}
					else
					{
						All_Nodes[std::pair<unsigned, unsigned>(x, y)] = label_count;
						Roots[label_count] = std::unique_ptr<Node<label>>(new Node<label>(label_count));
						++label_count;
					}
				}
			}
		}

		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<> uni(1, 255);
		//uni(rng)
		// for each label,
		// generate a color between 0-255
		// add to Label_Colors
		for (unsigned int i = 0; i < label_count; ++i)
		{
			std::vector<unsigned char>color;
			color.push_back((unsigned char)uni(rng));
			color.push_back((unsigned char)uni(rng));
			color.push_back((unsigned char)uni(rng));
			Label_Colors[i] = color;
		}

		// Loop thru xs and ys
		//All_Nodes.find != end
		// Get Label from All_Nodes
		// Use Label to get the root,
		// root to get the parent
		// get the label from parent
		// Get color using the label
		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				std::pair<unsigned, unsigned> coords(x, y);
				if (All_Nodes.find(coords) != All_Nodes.end())
				{
					label c_label = All_Nodes[coords];
					c_label = Roots[c_label]->Find(Roots[c_label].get())->val;
					std::vector<unsigned char> new_color = Label_Colors[c_label];
					added_image[(x * 3) + (y * (m_imageWidth * 3))] = new_color[0];
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = new_color[1];
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = new_color[2];
				}
				else // give it black
				{
					unsigned char new_color = 0;
					added_image[(x * 3) + (y * (m_imageWidth * 3))] = new_color;
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = new_color;
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = new_color;
				}
			}
		}

		ReplaceOriginal(added_image);
	}

	void Window::CreateHistogram(wxCommandEvent &)
	{
		std::map<unsigned char, double> Color_count;
		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				unsigned char color = c_image[(x * 3) + (y * (m_imageWidth * 3))];
				Color_count[color] += 1.0;
			}
		}

		std::vector<double> after_data(256);
		std::vector<double> original_data(256);
		for (auto iter = Color_count.begin(); iter != Color_count.end(); ++iter)
		{
			after_data[iter->first] = iter->second;
		}

		if (equalized)
		{
			std::map<unsigned char, double> Color_countOri;
			for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
			{
				for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
				{
					unsigned char color = be4_equalized[(x * 3) + (y * (m_imageWidth * 3))];
					Color_countOri[color] += 1.0;
				}
			}

			for (auto iter = Color_countOri.begin(); iter != Color_countOri.end(); ++iter)
			{
				original_data[iter->first] = iter->second;
			}
		}
		else
		{
			original_data = after_data;
		}

		HistogramChart* window = new HistogramChart(_("HistogramChart"), wxDefaultPosition, wxSize(1500, 1200), original_data, after_data, m_imageHeight * m_imageWidth);
		window->Show();
	}

	void Window::EqualizedHistogram(wxCommandEvent & )
	{
		using color_label = unsigned char;
		std::map<color_label, double> Color_count;
		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				unsigned char color = c_image[(x * 3) + (y * (m_imageWidth * 3))];
				Color_count[color] += 1.0;
			}
		}

		double mult = (255.0) / (double)(m_imageHeight * m_imageWidth);
		std::map<color_label, color_label> NewColors;

		for (unsigned i = 0; i <= 255; ++i)
		{
			color_label new_color;
			double current_value = 0.f;
			for (unsigned j = 0; j <= i; ++j)
			{
				if (Color_count.find(j) != Color_count.end())
					current_value += Color_count[j];
			}
			new_color = (color_label)std::min(std::max(std::round(current_value * mult),0.0), 255.0);
			NewColors[i] = new_color;
		}

		equalized = true;
		if (be4_equalized != nullptr)
			delete[] be4_equalized;

		be4_equalized = new unsigned char[m_totalSize];

		std::copy(c_image, c_image + m_totalSize, be4_equalized);

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				unsigned char color = c_image[(x * 3) + (y * (m_imageWidth * 3))];
				unsigned char new_color = NewColors[color];
				c_image[(x * 3) + (y * (m_imageWidth * 3))] = new_color;
				c_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = new_color;
				c_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = new_color;
			}
		}

		SetImgPanelSize(GetImgPanelSize());
	}

	void Window::FourierTransformSep(wxCommandEvent & ev)
	{
		time_t c_time = time(nullptr);
		unsigned char * f_image = new unsigned char[new_size];
		unsigned char * f_image_I = new unsigned char[new_size];

		//std::copy(new_image, new_image + new_size, f_image);

		using real = float;
		using imaginary = float;

		

		float max = std::numeric_limits<float>::min();
		float min = std::numeric_limits<float>::max();

		using Vs = unsigned;
		using Us = unsigned;
		using Xs = unsigned;
		using Ys = unsigned;
		using Exp = float;


		if (ev.GetId() == EV_M_FT_SEP)
		{
			std::map<Xs, std::map<Vs, std::complex<float>>> temp_data;

			for (unsigned v = 0; v < new_h; ++v)
			{
				for (unsigned y = 0; y < new_h; ++y)
				{
					for (unsigned x = 0; x < new_w; ++x)
					{
						float deta = -2.f* M_PI * v*y / new_h;
						temp_data[x][v] += new_image[(x * 3) + (y * (new_w * 3))] * powf(-1.f, (x + y)) * std::complex<float>(cosf(deta), sinf(deta));
					}
				}
			}

			std::vector<std::complex<float>> Fourier(new_w * new_h);
			std::vector<float> FourierMagnitude(new_w * new_h);

			std::vector<std::complex<float>> Fourier_Conjugate(new_w * new_h);

			for (unsigned v = 0; v < new_h; ++v)
			{
				for (unsigned u = 0; u < new_w; ++u)
				{
					for (unsigned x = 0; x < new_w; ++x)
					{
						float deta = -2.f* M_PI * u*x / new_w;
						Fourier[v * new_w + u] += temp_data[x][v] * std::complex<float>(cosf(deta), sinf(deta));
					}

					Fourier_Conjugate[v*new_w + u] = std::conj(Fourier[v*new_w + u]);

					float mag = sqrtf(std::norm(Fourier[v*new_w + u]));
					FourierMagnitude[v* new_w + u] = mag;
					if (mag > max)
						max = mag;
					if (mag < min)
						min = mag;
				}
			}

			for (unsigned u = 0; u < new_w; u++)
			{
				for (unsigned v = 0; v < new_h; v++)
				{
					float value = FourierMagnitude[u + v * new_w];
					f_image[(u * 3) + (v * (new_w * 3))] = ((value - min) / (max - min)) * 255;
					f_image[(u * 3) + (v * (new_w * 3)) + 1] = ((value - min) / (max - min)) * 255;
					f_image[(u * 3) + (v * (new_w * 3)) + 2] = ((value - min) / (max - min)) * 255;
				}
			}

			std::vector<std::complex<float>> Fourier_I(new_w * new_h);
			std::vector<float> FourierMagnitude_I(new_w * new_h);
			std::map<Xs, std::map<Vs, std::complex<float>>> temp_data_I;
			max = std::numeric_limits<float>::min();
			min = std::numeric_limits<float>::max();
			// Inverse
			for (unsigned v = 0; v < new_h; ++v)
			{
				for (unsigned y = 0; y < new_h; ++y)
				{
					for (unsigned x = 0; x < new_w; ++x)
					{
						float deta = -2.f* M_PI * v*y / new_h;
						temp_data_I[x][v] += Fourier_Conjugate[(x) + (y * (new_w))] /** powf(-1.f, (x + y))*/ * std::complex<float>(cosf(deta), sinf(deta));
					}
				}
			}

			//x_mag = powf(n_v.real() / (new_w * new_h), 2.0f);

			for (unsigned v = 0; v < new_h; ++v)
			{
				for (unsigned u = 0; u < new_w; ++u)
				{
					for (unsigned x = 0; x < new_w; ++x)
					{
						float deta = -2.f* M_PI * u*x / new_w;
						Fourier_I[v * new_w + u] += temp_data_I[x][v] * std::complex<float>(cosf(deta), sinf(deta));
					}

					float mag = sqrtf(std::norm(Fourier_I[v*new_w + u] / (float)(new_w * new_h) ));
					FourierMagnitude_I[v* new_w + u] = mag;
					if (mag > max)
						max = mag;
					if (mag < min)
						min = mag;
				}
			}

			for (unsigned u = 0; u < new_w; u++)
			{
				for (unsigned v = 0; v < new_h; v++)
				{
					float value = FourierMagnitude_I[u + v * new_w];
					f_image_I[(u * 3) + (v * (new_w * 3))] = ((value - min) / (max - min)) * 255;
					f_image_I[(u * 3) + (v * (new_w * 3)) + 1] = ((value - min) / (max - min)) * 255;
					f_image_I[(u * 3) + (v * (new_w * 3)) + 2] = ((value - min) / (max - min)) * 255;
				}
			}

		}

		c_time = time(nullptr) - c_time;
		wxString tw(std::to_string(c_time));
		FourierWindow* window = new FourierWindow(f_image, new_size, tw, this, -1, wxDefaultPosition, wxSize(new_w, new_h));

		//FourierWindow* window = new FourierWindow(f_image, new_size, _("FourierSpectrum"), this, -1, wxDefaultPosition, wxSize(new_w, new_h));
		window->Show();

		FourierWindow* window2 = new FourierWindow(f_image_I, new_size, tw, this, -1, wxDefaultPosition, wxSize(new_w, new_h));
		window2->Show();

		delete[] f_image;
		delete[] f_image_I;
	}

	unsigned int Window::BitReversal(unsigned int in, unsigned int size)
	{
		std::vector<int> tempVal(size);

		unsigned int currVal = in;

		for (unsigned int i = 0; i < size; ++i)
		{
			if ((currVal & 1) < 1)
			{
				tempVal[i] = 0;
			}
			else
			{
				tempVal[i] = 1;
			}

			currVal = currVal >> 1;
		}

		unsigned int reversedResult = 0;

		for (unsigned int j = 0; j < size; ++j)
		{
			reversedResult += tempVal[j] * std::powf(2.f, (size - 1 - j));
		}

		return reversedResult;
	}

	unsigned int ReverseBits(unsigned int input)
	{
		unsigned int output = input;
		for (int i = sizeof(input) * 8 - 1; i; --i)
		{
			output <<= 1;
			input >>= 1;
			output |= input & 1;
		}
		return output;
	}

	void Window::FFT(wxCommandEvent & ev)
	{
		//ResizeImage(unsigned char *& ori_image, unsigned char *& target_image, int c_width, int c_height, int w, int h, int new_totalSize)
		unsigned char * f_image;// = new unsigned char[new_size];
		unsigned f_width = new_w;
		unsigned f_height = new_h;

		f_width = (1 << (int)(std::log2(new_w) + 1));
		f_height = (1 << (int)(std::log2(new_h) + 1));

		unsigned f_size = f_width * f_height * 3;
		ResizeImage(new_image, f_image, new_w, new_h, f_width, f_height, f_size);

		int newBitSizeW = std::ceil(std::log2f(f_width));
		int newBitSizeH = std::ceil(std::log2f(f_height));

		//Slice horizontally
		std::vector < std::vector<std::complex<double>> > rows(f_height);

		for (unsigned y = 0; y < f_height; ++y)
		{
			for (unsigned x = 0; x < f_width; ++x)
			{
				rows[y].push_back(f_image[(x * 3) + (y * (f_width * 3))] * pow((-1), x + y));
			}
		}

		for (unsigned y = 0; y < f_height; ++y)
		{
			std::vector<std::complex<double>> temp_reverse(rows[y].size());
			for (unsigned j = 0; j < rows[y].size(); j++)
			{
				unsigned temp = BitReversal(j, newBitSizeW);
				temp_reverse[j] = rows[y][temp];
			}
			rows[y] = temp_reverse;
			cooley_tukey(rows[y].begin(), rows[y].end());
		}

		// Slice the image vertically
		std::vector < std::vector<std::complex<double>> >columns(f_width);

		for (unsigned x = 0; x < f_width; ++x)
		{
			for (unsigned y = 0; y < f_height; ++y)
			{
				columns[x].push_back(rows[y][x]);
			}
		}

		for (unsigned x = 0; x < f_width; ++x)
		{
			std::vector<std::complex<double>> temp_reverse(columns[x].size());
			for (unsigned j = 0; j < columns[x].size(); j++)
			{
				unsigned temp = BitReversal(j, newBitSizeH);
				temp_reverse[j] = columns[x][temp];
			}
			columns[x] = temp_reverse;
			cooley_tukey(columns[x].begin(), columns[x].end());
		}

		float max = std::numeric_limits<double>::min();
		float min = std::numeric_limits<double>::max();

		unsigned char * MagnitudeImage = new unsigned char[f_size];
		std::vector<float> FourierMagnitude(f_size);


		for (unsigned y = 0; y < f_height; ++y)
		{
			for (unsigned x = 0; x < f_width; ++x)
			{
				double mag = sqrtf(std::norm(log(columns[x][y])));

				FourierMagnitude[(x * 3) + (y * (f_width * 3))] = mag;

				if (mag < min)
					min = mag;
				if (mag > max)
					max = mag;
			}
		}

		for (unsigned u = 0; u < f_width; u++)
		{
			for (unsigned v = 0; v < f_height; v++)
			{
				float value = FourierMagnitude[(u * 3) + (v * (f_width * 3))];
				MagnitudeImage[(u * 3) + (v * (f_width * 3))] = ((value - min) / (max - min)) * 255;
				MagnitudeImage[(u * 3) + (v * (f_width * 3)) + 1] = ((value - min) / (max - min)) * 255;
				MagnitudeImage[(u * 3) + (v * (f_width * 3)) + 2] = ((value - min) / (max - min)) * 255;
			}
		}

		FourierWindow* window = new FourierWindow(MagnitudeImage, f_size, _("FourierSpectrum"), this, -1, wxDefaultPosition, wxSize(f_width, f_height));
		window->Show();

		//Do Inverse
		std::vector < std::vector<std::complex<double>> > rows_I(f_height);

		for (unsigned y = 0; y < f_height; ++y)
		{
			for (unsigned x = 0; x < f_width; ++x)
			{
				rows_I[y].push_back(std::conj(columns[x][y]) /** pow((-1), x + y)*/);
			}
		}

		for (unsigned y = 0; y < f_height; ++y)
		{
			std::vector<std::complex<double>> temp_reverse(rows_I[y].size());
			for (unsigned j = 0; j < rows_I[y].size(); j++)
			{
				unsigned temp = BitReversal(j, newBitSizeW);
				temp_reverse[j] = rows_I[y][temp];
			}
			rows_I[y] = temp_reverse;
			cooley_tukey(rows_I[y].begin(), rows_I[y].end());
		}

		// Slice the image vertically
		std::vector < std::vector<std::complex<double>> >columns_I(f_width);

		for (unsigned x = 0; x < f_width; ++x)
		{
			for (unsigned y = 0; y < f_height; ++y)
			{
				columns_I[x].push_back(rows_I[y][x]);
			}
		}

		for (unsigned x = 0; x < f_width; ++x)
		{
			std::vector<std::complex<double>> temp_reverse(columns_I[x].size());
			for (unsigned j = 0; j < columns_I[x].size(); j++)
			{
				unsigned temp = BitReversal(j, newBitSizeH);
				temp_reverse[j] = columns_I[x][temp];
			}
			columns_I[x] = temp_reverse;
			cooley_tukey(columns_I[x].begin(), columns_I[x].end());
		}

		max = std::numeric_limits<double>::min();
		min = std::numeric_limits<double>::max();

		unsigned char * MagnitudeImage_I = new unsigned char[f_size];
		std::vector<float> FourierMagnitude_I(f_size);


		for (unsigned y = 0; y < f_height; ++y)
		{
			for (unsigned x = 0; x < f_width; ++x)
			{
				double mag = sqrtf(std::norm(/*log*/(columns_I[x][y]/(double)(f_width * f_height))));

				FourierMagnitude_I[(x * 3) + (y * (f_width * 3))] = mag;

				if (mag < min)
					min = mag;
				if (mag > max)
					max = mag;
			}
		}

		for (unsigned u = 0; u < f_width; u++)
		{
			for (unsigned v = 0; v < f_height; v++)
			{
				float value = FourierMagnitude_I[(u * 3) + (v * (f_width * 3))];
				MagnitudeImage_I[(u * 3) + (v * (f_width * 3))] = ((value - min) / (max - min)) * 255;
				MagnitudeImage_I[(u * 3) + (v * (f_width * 3)) + 1] = ((value - min) / (max - min)) * 255;
				MagnitudeImage_I[(u * 3) + (v * (f_width * 3)) + 2] = ((value - min) / (max - min)) * 255;
			}
		}


		FourierWindow* window2 = new FourierWindow(MagnitudeImage_I, f_size, _("FourierSpectrum"), this, -1, wxDefaultPosition, wxSize(f_width, f_height));
		window2->Show();

		delete[] f_image;
		delete[] MagnitudeImage;
		delete[] MagnitudeImage_I;

	}

	void Window::GaussianNoise(wxCommandEvent & ev)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Mean and Variance, separated by spaces "));
		std::string text = inputtext;
		std::stringstream text_s(text);

		unsigned int mean = 0, var = 0;
		text_s >> mean;
		text_s >> var;

		std::default_random_engine generator;
		std::normal_distribution<float> dist(mean, sqrt(var));

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				float mag = c_image[(x * 3) + (y * (m_imageWidth * 3))] + dist(generator);
				mag = std::min(std::max(mag, 0.f), 255.0f);
				added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
			}
		}

		ReplaceOriginal(added_image);
	}

	void Window::SaltPepper(wxCommandEvent & ev)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Probabilites, separated by spaces"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		float lower = 0, upper = 0;
		text_s >> lower;
		text_s >> upper;

		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine generator(seed);

		std::uniform_real_distribution<float> dist(0.0, 1.0);
		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				float prob = dist(generator);
				float mag = c_image[(x * 3) + (y * (m_imageWidth * 3))];
				if (prob < lower)
					mag = 0.f;
				else if (prob > upper)
					mag = 255.f;
				
				mag = std::min(std::max(mag, 0.f), 255.0f);
				added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
			}
		}

		ReplaceOriginal(added_image);

	}

	void Window::Median(wxCommandEvent & ev)
	{
		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				std::vector<unsigned> neighborhood;
				for (int a = 0; a < 3; ++a)
				{
					for (int b = 0; b < 3; ++b)
					{
						int xn = x + a - 1;
						int yn = y + b - 1;

						int index = (xn * 3) + (yn * (m_imageWidth * 3));

						if (xn< 0 || xn >= m_imageWidth || yn<0 || yn >= m_imageHeight)
						{
						}
						else
						{
							neighborhood.push_back(c_image[index]);
						}
					}
				}

				std::sort(neighborhood.begin(), neighborhood.end());
				unsigned half = neighborhood.size() / 2;
				
				float mag = neighborhood[half];
				mag = std::min(std::max(mag, 0.f), 255.0f);

				added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
			}
		}

		ReplaceOriginal(added_image);
	}

	void Window::LocalNoiseReduction(wxCommandEvent & ev)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Std Deviation"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		float var = 0;
		text_s >> var;
		var = powf(var, 2.0f);

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				std::vector<unsigned> neighborhood;
				for (int a = 0; a < 3; ++a)
				{
					for (int b = 0; b < 3; ++b)
					{
						int xn = x + a - 1;
						int yn = y + b - 1;

						int index = (xn * 3) + (yn * (m_imageWidth * 3));

						if (xn< 0 || xn >= m_imageWidth || yn<0 || yn >= m_imageHeight)
						{
						}
						else
						{
							neighborhood.push_back(c_image[index]);
						}
					}
				}

				unsigned cur_val = c_image[(x * 3) + (y * (m_imageWidth * 3))];

				float var_neighborhood = 0.f;
				float mean_neighborhood = std::accumulate(neighborhood.begin(), neighborhood.end(), 0.f);
				mean_neighborhood /= (float)neighborhood.size();

				for (auto & n : neighborhood)
				{
					var_neighborhood += powf(n - mean_neighborhood, 2.0f);
				}
				var_neighborhood /= (float)neighborhood.size();



				float mag = cur_val - (var / var_neighborhood) * (cur_val - mean_neighborhood);
				mag = std::min(std::max(mag, 0.f), 255.0f);

				added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
			}
		}

		ReplaceOriginal(added_image);
	}

	void Window::AdaptiveMedianNoiseReduction(wxCommandEvent & ev)
	{
		wxString inputtext = wxGetTextFromUser(wxT("Enter Max Filter Size"));
		std::string text = inputtext;
		std::stringstream text_s(text);

		int max_filt_size = 0;
		text_s >> max_filt_size;
		int filt_size = 3;

		while (max_filt_size <= 0 || max_filt_size > m_imageHeight || max_filt_size > m_imageWidth)
		{
			inputtext = wxGetTextFromUser(wxT("Enter Max Filter, Must be Positive"));
			text = inputtext;
			std::stringstream text_s1(text);

			text_s1 >> max_filt_size;
		}

		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (int x = 0; x < (int)m_imageWidth; ++x) // column
		{
			for (int y = 0; y < (int)m_imageHeight; ++y) //row
			{
				std::vector<unsigned> neighborhood;
				for (int a = 0; a < filt_size; ++a)
				{
					for (int b = 0; b < filt_size; ++b)
					{
						int xn = x + a - 1;
						int yn = y + b - 1;

						int index = (xn * 3) + (yn * (m_imageWidth * 3));

						if (xn< 0 || xn >= m_imageWidth || yn<0 || yn >= m_imageHeight)
						{
						}
						else
						{
							neighborhood.push_back(c_image[index]);
						}
					}
				}

				std::sort(neighborhood.begin(), neighborhood.end());
				unsigned half = neighborhood.size() / 2;
				float median = neighborhood[half];
				float max = neighborhood.back();
				float min = neighborhood.front();

				float A1 = median - min;
				float A2 = median - max;

				float mag = 0.f;
				bool output = false;

				if (A1 > 0 && A2 < 0)
				{
					mag = StageB(neighborhood, min, max, median, c_image[(x * 3) + (y * (m_imageWidth * 3))]);
					output = true;
					filt_size = 3;
				}
				else
				{
					filt_size++;
					if (filt_size <= max_filt_size)
					{
						output = false;
					}
					else
					{
						output = true;
						filt_size = 3;
						mag = median;
					}
				}

				if (output)
				{
					mag = std::min(std::max(mag, 0.f), 255.0f);
					added_image[(x * 3) + (y * (m_imageWidth * 3))] = mag;
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = mag;
					added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = mag;
				}
				else
				{
					--y;
				}
			}
		}

		ReplaceOriginal(added_image);
	}

	
	float Window::StageB(std::vector<unsigned> neighborhood, float zmin, float zmax, float zmedian, float cur_val)
	{
		float B1 = cur_val - zmin;
		float B2 = cur_val - zmax;

		if (B1 > 0.f && B2 < 0.f)
			return cur_val;
		else
			return zmedian;
	}

	void Window::Mean(wxCommandEvent & ev)
	{
		unsigned char * added_image = nullptr;
		added_image = new unsigned char[m_totalSize];

		for (unsigned x = 0; x < (unsigned)m_imageWidth; ++x) // column
		{
			for (unsigned y = 0; y < (unsigned)m_imageHeight; ++y) //row
			{
				float magX = 0.f;

				for (int a = 0; a < 3; ++a)
				{
					for (int b = 0; b < 3; ++b)
					{
						int xn = x + a - 1;
						int yn = y + b - 1;

						int index = (xn * 3) + (yn * (m_imageWidth * 3));

						if (xn< 0 || xn >= m_imageWidth || yn<0 || yn >= m_imageHeight)
						{
						}
						else
						{
							magX += (float) c_image[index];
						}
					}
				}

				magX /= 9.f;
				added_image[(x * 3) + (y * (m_imageWidth * 3))] = magX;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 1] = magX;
				added_image[(x * 3) + (y * (m_imageWidth * 3)) + 2] = magX;
			}
		}

		ReplaceOriginal(added_image);
	}
	

	void Window::FourierTransform(wxCommandEvent & ev)
	{
		time_t c_time = time(nullptr);
		unsigned char * f_image = new unsigned char[new_size];
		unsigned char * f_image_inverse = new unsigned char[new_size];

		std::copy(new_image, new_image + new_size, f_image);

		using real = float;
		using imaginary = float;

		std::vector<std::pair<real, imaginary>> Fourier_Image(new_size);
		std::vector<std::pair<real, imaginary>> FourierConjugate;

		std::vector<float> FourierMagnitude(new_size);
		std::vector<std::pair<real, imaginary>> FourierInverse(new_size);

		float max = std::numeric_limits<float>::min();
		float min = std::numeric_limits<float>::max();

		if (ev.GetId() == EV_M_FT_DIRECT)
		{
			for (unsigned u = 0; u < new_w; u++)
			{
				for (unsigned v = 0; v < new_h; v++)
				{
					// For each new pixel
					std::pair<real, imaginary> new_value(0.f,0.f);
					for (unsigned x = 0; x < (unsigned)new_w; ++x)
					{
						for (unsigned y = 0; y < (unsigned)new_h; ++y)
						{
							float value = (float) new_image[(x * 3) + (y * (new_w* 3))] * powf((-1), x+ y);
							float real_value = cosf(-2.0f * M_PI*(((float)u*x / (float)new_w) + ((float)v*y / (float)new_h)));
							float imag_value = sinf(-2.0f * M_PI*(((float)u*x / (float)new_w) + ((float)v*y / (float)new_h)));

							new_value.first += value * real_value;
							new_value.second += value * imag_value;

						}
					}
					// For each new pixel

					Fourier_Image[(u * 3) + (v * (new_w * 3))] = new_value;
					Fourier_Image[(u * 3) + (v * (new_w * 3)) + 1] = new_value;
					Fourier_Image[(u * 3) + (v * (new_w * 3))+ 2] = new_value;

					float mag = sqrtf(powf(new_value.first, 2.0f) + powf(new_value.second, 2.0f));
					FourierMagnitude[(u * 3) + (v * (new_w * 3))] = mag;
					FourierMagnitude[(u * 3) + (v * (new_w * 3)) + 1] = mag;
					FourierMagnitude[(u * 3) + (v * (new_w * 3)) + 2] = mag;

					if (mag < min)
						min = mag;
					if (mag > max)
						max = mag;

				}
			}

			 //Get Fourier Conjuagate
			for (auto &a : Fourier_Image)
			{
				FourierConjugate.push_back(std::pair<float, float>(a.first, -a.second));
			}

			// Perform DFT on Conjugate
			for (unsigned u = 0; u < new_w; u++)
			{
				for (unsigned v = 0; v < new_h; v++)
				{
					std::complex<float> n_v;
					for (unsigned x = 0; x < (unsigned)new_w; ++x)
					{
						for (unsigned y = 0; y < (unsigned)new_h; ++y)
						{
							float value = (float)FourierConjugate[(x * 3) + (y * (new_w * 3))].first/* / powf((-1), x + y)*/;
							float value2 = (float)FourierConjugate[(x * 3) + (y * (new_w * 3))].second /*/ powf((-1), x + y)*/;

							std::complex<float> cur(value, value2);

							float real_value = cosf(-2.0f * M_PI*(((float)u*x / (float)new_w) + ((float)v*y / (float)new_h)));
							float imag_value = sinf(-2.0f * M_PI*(((float)u*x / (float)new_w) + ((float)v*y / (float)new_h)));

							n_v += cur * std::complex<float>(real_value, imag_value);

						}
					}
					// For each new pixel

					float mag;
					float x_mag = powf(n_v.real() / (new_w * new_h), 2.0f);
					float y_mag = powf(n_v.imag() / (new_w * new_h), 2.0f);

					mag = sqrt(x_mag + y_mag);

					f_image_inverse[(u * 3) + (v * (new_w * 3))] = mag;// new_value.first;
					f_image_inverse[(u * 3) + (v * (new_w * 3)) + 1] = mag;// new_value.first;
					f_image_inverse[(u * 3) + (v * (new_w * 3)) + 2] = mag;// new_value.first;

				}
			}

			for (unsigned u = 0; u < new_w; u++)
			{
				for (unsigned v = 0; v < new_h; v++)
				{
					float value = FourierMagnitude[(u * 3) + (v * (new_w * 3))];

					f_image[(u * 3) + (v * (new_w * 3))] = ((value - min) / (max - min)) * 255;
					f_image[(u * 3) + (v * (new_w * 3)) + 1] = ((value - min) / (max - min)) * 255;
					f_image[(u * 3) + (v * (new_w * 3)) + 2] = ((value - min) / (max - min)) * 255;
				}
			}

		}

		c_time = time(nullptr) - c_time;
		wxString tw(std::to_string(c_time));
		FourierWindow* window = new FourierWindow(f_image, new_size, tw, this, -1, wxDefaultPosition, wxSize(new_w, new_h));
		window->Show();

		FourierWindow* window2 = new FourierWindow(f_image_inverse, new_size, _("FourierSpectrum"), this, -1, wxDefaultPosition, wxSize(new_w, new_h));
		window2->Show();

		delete[] f_image_inverse;
		delete[] f_image;
	}

	std::vector<std::pair<unsigned, unsigned>> Window::GetNeighbors(unsigned x, unsigned y
		, unsigned char min, unsigned char max)
	{
		std::vector<std::pair<unsigned, unsigned>> Neighbors;

		// Try get North
		unsigned n_x = x;
		unsigned n_y = y - 1;
		bool North = false;
		// Check within bounds, Check within min & max, 
		if (IsNeighbor(n_x, n_y, min, max))
		{
			Neighbors.push_back(std::pair<unsigned, unsigned>(n_x, n_y));
			North = true;
		}

		// Try get West
		unsigned w_x = x - 1;
		unsigned w_y = y;
		bool West = false;
		// Check within bounds, Check within min & max, 
		if (IsNeighbor(w_x, w_y, min, max))
		{
			Neighbors.push_back(std::pair<unsigned, unsigned>(w_x, w_y));
			West = true;
		}

		// Get NE an NW
		if (!North && !West)
		{
			// Try get North East
			unsigned ne_x = x + 1;
			unsigned ne_y = y - 1;
			// Check within bounds, Check within min & max, 
			if (/*!North &&*/ IsNeighbor(ne_x, ne_y, min, max))
			{
				Neighbors.push_back(std::pair<unsigned, unsigned>(ne_x, ne_y));
			}

			// Try get North West
			unsigned nw_x = x - 1;
			unsigned nw_y = y - 1;
			// Check within bounds, Check within min & max, 
			if (/*!North &&*/ !West && IsNeighbor(nw_x, nw_y, min, max))
			{
				Neighbors.push_back(std::pair<unsigned, unsigned>(nw_x, nw_y));
			}
		}

		return Neighbors;

	}

	// Check if in image bounds and color in range
	bool Window::IsNeighbor(unsigned x, unsigned y, unsigned char min, unsigned char max)
	{
		if (x >= (unsigned)0 && x < (unsigned)m_imageWidth && y >= (unsigned)0 && y < (unsigned)m_imageHeight)
		{
			unsigned char color = c_image[(x * 3) + (y * (m_imageWidth * 3))];
			if (color >= min && color <= max)
			{
				return true;
			}
		}
		return false;
	}

	void Window::new_staticbmp(unsigned char* image, int w, int h)
	{
		if (m_imageRGB != nullptr)
		delete m_imageRGB;
		m_imageRGB = new wxImage(w, h, image);

		if (m_imageBitmap != nullptr)
		delete m_imageBitmap;
		m_imageBitmap = new wxBitmap(*m_imageRGB);
		if (sbmp != nullptr)
			delete sbmp;
		sbmp = new wxStaticBitmap(this, wxID_STATIC, wxNullBitmap, wxDefaultPosition, wxSize(w, h));
		sbmp->SetBitmap(*m_imageBitmap);
		sbmp->SetPosition(wxPoint(0, 0));
	}

	FourierWindow::~FourierWindow()
	{
		if(m_sbmp)
			delete m_sbmp;
		//if (temp_imageBitmap)
		delete temp_imageBitmap;

		delete temp_image;
		//delete m_image;
	}

	FourierWindow::FourierWindow(unsigned char *image, unsigned totalSize, const wxString& title, wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
		:wxFrame(parent, id, title, pos, size)
	{
		m_image = new unsigned char[totalSize];
		std::copy(image, image + totalSize, m_image);
		temp_image = new wxImage(size.x, size.y, m_image);
		temp_imageBitmap = new wxBitmap(*temp_image);
		m_sbmp = new wxStaticBitmap(this, wxID_STATIC, wxNullBitmap, wxDefaultPosition, size);
		m_sbmp->SetBitmap(*temp_imageBitmap);
		m_sbmp->SetPosition(wxPoint(0, 0));
	}


	/// Create event table
	BEGIN_EVENT_TABLE(Window, wxFrame)
		EVT_MENU(EV_M_FILE_OPEN, Window::OnFileOpen)
		EVT_MENU(EV_M_FILE_EXIT, Window::OnExit)
		EVT_MENU(EV_M_HELP_ABOUT, Window::OnAbout)
		EVT_MENU(EV_M_IMG_NN, Window::OnImgNN)
		EVT_MENU(EV_M_IMG_BILINEAR, Window::OnImgBilinear)
		EVT_MENU(EV_M_FILE_SAVE, Window::OnFileSave)
		EVT_MENU(EV_M_FILE_SAVEAS, Window::OnFileSaveAs)
		EVT_MENU(EV_M_IMG_ADD, Window::AddSubtractProduct)
		EVT_MENU(EV_M_IMG_SUBSTRACT, Window::AddSubtractProduct)
		EVT_MENU(EV_M_IMG_PRODUCT, Window::AddSubtractProduct)
		EVT_MENU(EV_M_IMG_NEGATIVE, Window::Negative)
		EVT_MENU(EV_M_IMG_LOGTRANSFORM, Window::LogTransform)
		EVT_MENU(EV_M_IMG_POWERTRANSFORM, Window::PowerTransform)
		EVT_MENU(EV_M_IMG_GAUSSIAN, Window::GaussianFilter)
		EVT_MENU(EV_M_IMG_UNSHARP, Window::UnsharpMask)
		EVT_MENU(EV_M_IMG_SOBEL, Window::SobelFilter)
		EVT_MENU(EV_M_CONNECTED_COMPONENT, Window::MConnectedLabelling)
		EVT_MENU(EV_M_VIEWHISTOGRAMS, Window::CreateHistogram)
		EVT_MENU(EV_M_EQUALIZE, Window::EqualizedHistogram)
		EVT_MENU(EV_M_FT_DIRECT, Window::FourierTransform)
		EVT_MENU(EV_M_FT_SEP, Window::FourierTransformSep)
		EVT_MENU(EV_M_FT_FFT, Window::FFT)
		EVT_MENU(EV_M_GAUSSIAN_NOISE, Window::GaussianNoise)
		EVT_MENU(EV_M_SALT_PEPPER, Window::SaltPepper)
		EVT_MENU(EV_M_MEDIAN, Window::Median)
		EVT_MENU(EV_M_LOCAL_NOISE, Window::LocalNoiseReduction)
		EVT_MENU(EV_M_ADAPTIVE, Window::AdaptiveMedianNoiseReduction)
		EVT_MENU(EV_M_MEAN, Window::Mean)
		EVT_SIZE(Window::OnResize)
		END_EVENT_TABLE()

		BEGIN_EVENT_TABLE(HistogramChart, wxFrame)
		EVT_PAINT(HistogramChart::OnPaint)
		END_EVENT_TABLE()
} // namespace cs370
