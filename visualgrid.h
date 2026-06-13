#include <vector>
#include <mutex>
#include <wx/wx.h>
#include <fstream>

#include "utils.h"

class VisualGrid : public wxWindow
{
public:
    VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int xSquaresCount, std::vector<float> &v, std::mutex &m, std::ofstream &file);

private:
    void OnPaint(wxPaintEvent &evt);
    void OnResizeWindow(wxSizeEvent &evt);

    int NumberOfPoints;
    int w, h;
    std::ofstream &logfile;

    std::mutex &valuesMutex;
    const std::vector<float> &values;
};