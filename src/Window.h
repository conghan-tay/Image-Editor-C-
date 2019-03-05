#ifndef _WINDOW_H
#define _WINDOW_H
#include <map>
#include <random>
#include <memory>

namespace cs370
{
	template <typename T>
	struct Node
	{
		Node(const T& val = 0) : parent(this), rank(0), val(val) {}

		Node<T>* Find(Node<T>* x)
		{
			if (x->parent != x)
				x->parent = Find(x->parent);
			return x->parent;
		}

		void Union(Node<T> *y)
		{
			Node<T> *xRoot = Find(this);
			Node<T> *yRoot = Find(y);

			if (xRoot == yRoot) return;

			if (xRoot->rank < yRoot->rank)
			{
				xRoot->parent = yRoot;
			}
			else if (xRoot->rank > yRoot->rank)
			{
				yRoot->parent = xRoot;
			}
			else
			{
				yRoot->parent = xRoot;
				xRoot->rank = xRoot->rank + 1;
			}
		}

		Node<T> *parent;
		int rank;
		T val;
	};

	class Image;

	enum WindowEvents
	{
		EV_TEXTBOX,
		EV_HIST1,

		// Menu events
		EV_M_FILE_OPEN,
		EV_M_FILE_SAVE,
		EV_M_FILE_SAVEAS,
		EV_M_FILE_EXIT,
		EV_M_IMG_NN,
		EV_M_IMG_BILINEAR,
		EV_M_IMG_REVERT,
		EV_M_IMG_PROPERTIES,
		EV_M_HELP_HELP,
		EV_M_HELP_ABOUT,
		EVT_SIZE,

		// Image Operations
		EV_M_IMG_ADD,
		EV_M_IMG_SUBSTRACT,
		EV_M_IMG_PRODUCT,
		EV_M_IMG_NEGATIVE,
		EV_M_IMG_LOGTRANSFORM,
		EV_M_IMG_POWERTRANSFORM,
		EV_M_IMG_GAUSSIAN,
		EV_M_IMG_SOBEL,
		EV_M_IMG_UNSHARP,
		EV_M_CONNECTED_COMPONENT,
		EV_M_VIEWHISTOGRAMS,
		EV_M_EQUALIZE,
		EV_M_GAUSSIAN_NOISE,
		EV_M_SALT_PEPPER,
		EV_M_MEDIAN,
		EV_M_LOCAL_NOISE,
		EV_M_ADAPTIVE,
		EV_M_MEAN,


		EV_M_FT_DIRECT,
		EV_M_FT_SEP,
		EV_M_FT_FFT,

		EV_NUM_EVENTS
	}; // enum WindowEvents


	enum class PPMTYPE { P3, P6 };
	enum class INTERPOLANT { NN, BL };

	class Window : public wxFrame
	{
		using label = unsigned;
	public:
		Window(const wxString& title, const wxPoint& pos, const wxSize& size);
		virtual ~Window();

		wxSize GetImgPanelSize() const;
		void SetImgPanelSize(const wxSize& size);
		void SetImgPanelSize(int w, int h);
		void ReplaceOriginal(unsigned char * replacement);


		void OnResize(wxSizeEvent& ev);

		// Menu handlers
		void OnFileOpen(wxCommandEvent& ev);
		void OnFileSave(wxCommandEvent& ev);
		void OnFileSaveAs(wxCommandEvent& ev);
		void OnExit(wxCommandEvent& ev);
		void OnImgNN(wxCommandEvent& ev);
		void OnImgBilinear(wxCommandEvent& ev);
		void OnAbout(wxCommandEvent& ev);

		void LoadImageFromFile(wxString& file_name, unsigned char* &image, int &width, int &height, int &totalSize);
		void ResizeImage(unsigned char *& ori_image, unsigned char *& target_image, int c_width, int c_height, int w, int h, int new_totalSize);
		// Image Operations
		void AddSubtractProduct(wxCommandEvent& ev);
		void Negative(wxCommandEvent& ev);
		void LogTransform(wxCommandEvent& ev);
		void PowerTransform(wxCommandEvent& ev);
		void GaussianFilter(wxCommandEvent& ev);
		void SobelFilter(wxCommandEvent& ev);
		void UnsharpMask(wxCommandEvent& ev);
		void MConnectedLabelling(wxCommandEvent& ev);
		void CreateHistogram(wxCommandEvent & ev);
		void EqualizedHistogram(wxCommandEvent & ev);
		void FourierTransform(wxCommandEvent & ev);
		void FourierTransformSep(wxCommandEvent & ev);
		void FFT(wxCommandEvent & ev);
		void GaussianNoise(wxCommandEvent & ev);
		void SaltPepper(wxCommandEvent & ev);
		void Median(wxCommandEvent & ev);
		void LocalNoiseReduction(wxCommandEvent & ev);
		void AdaptiveMedianNoiseReduction(wxCommandEvent & ev);
		float StageB(std::vector<unsigned> neighborhood, float zmin, float zmax, float zmedian, float cur_val);
		void Mean(wxCommandEvent & ev);

		unsigned int BitReversal(unsigned int in, unsigned int size);

		bool equalized = false;
		unsigned char * be4_equalized;

		std::vector<std::pair<unsigned, unsigned>>
			GetNeighbors(unsigned x, unsigned y, unsigned char min, unsigned char max);

		bool IsNeighbor(unsigned x, unsigned y, unsigned char min, unsigned char max);

		DECLARE_EVENT_TABLE()

	private:
		INTERPOLANT m_interP;
		//wxImagePanel* imgpanel_;
		wxTextCtrl* cmdBox_;
		wxTextCtrl *MainEditBox;
		wxMenu*     imgMenu_;

		// Textbox vars
		std::string pwd_; ///< present working directory
		wxString CurrentDocPath;

		//
		int m_imageWidth;
		int m_imageHeight;
		int m_totalSize;
		wxBitmap *m_imageBitmap;	// used to display the image
		wxImage *m_imageRGB;		// used to load the image
		wxStaticBitmap* sbmp;
		unsigned char* c_image;

		unsigned char * new_image;

		// for unsharp mask
		unsigned char * unsharp_gaussian;
		unsigned char * be4gauss_image;

		int new_w;
		int new_h;
		int new_size;

		PPMTYPE ppmt;
		void new_staticbmp(unsigned char* image, int w, int h);

		wxMenu* filemenu;
		wxMenu* helpmenu;
		wxMenuBar* menubar;

		//wxTextEntryDialog  *logConstant;

		unsigned m_colorRange;

	}; // class Window

	class HistogramChart : public wxFrame
	{
	public:
		HistogramChart(const wxString& title, const wxPoint& pos, const wxSize& size, std::vector<double>& L, std::vector<double>& newL, float pixel_count)
			: wxFrame(nullptr, -1, title, pos, size), hist1(L), hist2(newL), pixel_count(pixel_count)
		{
			// Set window dimensions
			const wxSize clientsize(this->GetClientSize());
			//imgpanel_ = new wxPanel(this, -1, wxPoint(0, 600), wxSize(clientsize.GetWidth(), 1000));




			//imgpanel_->Hide();

		}
		virtual ~HistogramChart()
		{

		}
		void OnPaint(wxPaintEvent&)
		{
			wxPaintDC dc(this);



			// draw a circle
			//dc.SetBrush(*wxGREEN_BRUSH); // green filling
			//dc.SetPen(wxPen(wxColor(255, 0, 0), 5)); // 5-pixels-thick red outline
			//dc.DrawCircle(wxPoint(200, 100), 25 /* radius */);
			//
			// draw a rectangle
			//dc.SetBrush(*wxBLUE_BRUSH); // blue filling
			dc.SetPen(wxPen(wxColor(255, 255, 255), 1)); // 10-pixels-thick pink outline
			dc.DrawRectangle(10, 0, 1500, 500);
			dc.DrawRectangle(10, 500, 1500, 500);

			// draw a line
			dc.SetPen(wxPen(wxColor(255, 0, 0), 5)); // black line, 3 pixels thick

			for (int i = 0; i < 256; ++i)
				dc.DrawLine(i * 5 + 10, 500, i * 5 + 10, 500 - int(3000.0f * (float)hist1[i] / pixel_count/*hist1.size()*/));
			//dc.DrawLine(0, i*2, int(hist1[i]/ hist1.size()), i*2); // draw line across the rectangle
			dc.SetPen(wxPen(wxColor(0, 0, 0), 5)); // black line, 3 pixels thick
			for (int i = 0; i < 256; ++i)
				dc.DrawLine(i * 5 + 10, 1000, i * 5 + 10, 1000 - int(3000.0f * (float)hist2[i] / pixel_count/*hist1.size()*/));

			dc.DrawText(wxT("Original"), 10, 5);
			dc.DrawText(wxT("Equalized"), 10, 505);
			//dc.DrawLine(300, i*2, 300+ int(hist2[i]/ hist2.size()), i*2); // draw line across the rectangle
			//wxImage tempImage(510, 255, image.data(), true);
			//image_bitmap = wxBitmap(tempImage);
			//dc.DrawBitmap(image_bitmap, 0, 0, 0);

		}

		DECLARE_EVENT_TABLE()
	private:
		wxPanel * imgpanel_;
		wxBitmap image_bitmap;
		std::vector<unsigned char> image;
		std::vector<double> hist1, hist2;
		float pixel_count;

	};

	class FourierWindow : public wxFrame {
	public:
		unsigned char * m_image;
		wxStaticBitmap* m_sbmp;
		wxImage * temp_image;
		wxBitmap * temp_imageBitmap;
		~FourierWindow();
		FourierWindow(unsigned char *image, unsigned totalSize, const wxString& title, wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size);
		//DECLARE_EVENT_TABLE()
	};

} // namespace cs370

#endif /* _WINDOW_H */
