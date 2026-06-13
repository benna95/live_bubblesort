#include "visualgrid.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

VisualGrid::VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int NumberOfPoints, std::vector<float> &v, std::mutex &m, std::ofstream &file) : wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE), NumberOfPoints(NumberOfPoints), values(v), valuesMutex(m), h(size.GetHeight()), w(size.GetWidth()), logfile(file)
{
    this->SetBackgroundStyle(wxBG_STYLE_PAINT);
    this->Bind(wxEVT_PAINT, &VisualGrid::OnPaint, this);
    //this->Bind(wxEVT_SIZE, &VisualGrid::OnResizeWindow, this); // in alternativa posso usare GetClientSize, vedi riga 19
}

void VisualGrid::OnPaint(wxPaintEvent&)
{
    DEBUG_LOG("appena entrato dentro OnPaint\n", logfile);
    wxAutoBufferedPaintDC dc(this);
    auto gc = std::unique_ptr<wxGraphicsContext>(wxGraphicsContext::Create(dc));
    DEBUG_LOG("contesto grafico creato\n", logfile);
    //wxGraphicsContext* gc = wxGraphicsContext::CreateFromUnknownDC(dc);
    if (!gc) return;

    GetClientSize(&w, &h);
    float barWidth = static_cast<float>(w) / NumberOfPoints;
    float maxHeight = static_cast<float>(h);
   
    // resetto il background
    gc->SetBrush(*wxBLACK_BRUSH);
    gc->DrawRectangle(0, 0, w, h);
    DEBUG_LOG("background resettato\n", logfile);

    std::lock_guard lk(valuesMutex);
    DEBUG_LOG("mutex per il rendering bloccato\n", logfile);
    
    for (size_t i = 0; i < values.size(); ++i)
    {
        int   intensity = static_cast<int>(values[i] * 255);

        // rgb
        int r = 255 * values[i];
        int g = 255 * (1 - std::abs(values[i] - 0.5f) * 2);
        int b = 255 * (1 - values[i]);

        wxColour color(r, g, b);
        gc->SetBrush(wxBrush(color));

        float barHeight = values[i] * maxHeight;
        gc->DrawRectangle(i * barWidth,     h - barHeight,  barWidth - 0.5f,    barHeight);
    }

    //delete gc;
    DEBUG_LOG("mutex per il rendering rilasciato\n", logfile);
}

void VisualGrid::OnResizeWindow(wxSizeEvent& e)
{
    h = this->GetSize().GetHeight();
    w = this->GetSize().GetWidth();
    Refresh();
    e.Skip();
}