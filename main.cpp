#include <wx/wx.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <random>
#include <fstream>

#include "visualgrid.h"
#include "utils.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    virtual ~MyFrame();

private:
    wxButton *button;
    wxButton *RandomizeDataButton;

    wxGauge *progressBar;
    VisualGrid *grid;

    bool processing{false};
    std::atomic<bool> quitRequested{false};

    std::vector<float> sharedData;
    std::mutex dataMutex;

    std::thread backgroundThread;
    std::ofstream logfile;

    void OnButtonClick(wxCommandEvent &e);
    void OnClose(wxCloseEvent &e);
    void OnTime(wxTimerEvent& e);
    void OnResizeWindow(wxSizeEvent& e);

    void BackgroundTask();
    void RandomizeSharedData(wxCommandEvent& e);

    wxTimer* refreshTimer = nullptr;
    static constexpr int RefreshTimerId = 6612;
};

#ifdef _DEBUG
wxIMPLEMENT_APP_CONSOLE(MyApp);
#else
wxIMPLEMENT_APP(MyApp);
#endif 


bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame("Bubblesort", wxPoint(300, 95), wxDefaultSize);
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size), sharedData(10000)
{
    //RandomizeSharedData();
    this->CreateStatusBar(1);

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    wxSizer *panelSizer = new wxBoxSizer(wxHORIZONTAL);

    button = new wxButton(panel, wxID_ANY, "Start");
    button->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);
    button->Disable();

    RandomizeDataButton = new wxButton(panel, wxID_ANY, "Randomize Data");
    RandomizeDataButton->Bind(wxEVT_BUTTON, &MyFrame::RandomizeSharedData, this);

    progressBar = new wxGauge(panel, wxID_ANY, 1000, wxDefaultPosition, FromDIP(wxSize(900, 20)));

    panelSizer->Add(button, 0, wxALIGN_CENTER, FromDIP(5));
    panelSizer->Add(RandomizeDataButton, 0, wxALIGN_CENTER, FromDIP(10));
    panelSizer->Add(progressBar, 1, wxALIGN_CENTER);
    logfile = std::ofstream("log.txt", std::ios::out); // se riapro il file, cancello il contenuto

    grid = new VisualGrid(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(600, 600)), sharedData.size(), sharedData, dataMutex, logfile);

    panel->SetSizer(panelSizer);

    sizer->Add(panel, 0, wxEXPAND | wxALL, FromDIP(5));
    sizer->Add(grid, 1, wxEXPAND | wxALL, FromDIP(5));

    this->SetSizerAndFit(sizer);

    this->Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

    this->SetStatusText("Randomize data before sorting...");

    this->refreshTimer = new wxTimer(this, RefreshTimerId);
    this->Bind(wxEVT_TIMER, &MyFrame::OnTime, this, RefreshTimerId);

	this->refreshTimer->Start(25);

    this->Bind(wxEVT_SIZE, &MyFrame::OnResizeWindow, this);

    std::stringstream oss;
    oss << "1. Main thread partito" << std::this_thread::get_id() << std::endl;
    DEBUG_LOG(oss.str(), logfile);
    
}

void MyFrame::OnButtonClick(wxCommandEvent &e)
{
    if (!this->processing)
    {
        this->processing = true;

        this->backgroundThread = std::thread{&MyFrame::BackgroundTask, this};
    }
}

void MyFrame::OnClose(wxCloseEvent& e)
{
    if (refreshTimer)
    {
        refreshTimer->Stop();
    }

    if (processing)
    {
        e.Veto();
        quitRequested = true;
    }
    else
    {
        DEBUG_LOG("chiamo Destroy\n", logfile);
        Destroy();
        DEBUG_LOG("Destroy chiamato\n", logfile);
        
    }
}

void MyFrame::RandomizeSharedData(wxCommandEvent &e)
{   
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(0, 1);

    for (int i = 0; i < sharedData.size(); i++)
    {
        sharedData[i] = distr(gen);
    }
    //wxMessageBox("data randomized correctly");
    DEBUG_LOG("Randomize data performed\n", logfile);
    button->Enable();
}

void MyFrame::BackgroundTask()
{
    std::ostringstream oss;
    oss << "2. Background thread partito" << std::this_thread::get_id() << std::endl;
    DEBUG_LOG(oss.str(), logfile);

    int n = sharedData.size();
    wxGetApp().CallAfter([this, n]()
                         {
                                     this->SetStatusText(wxString::Format("Sorting the array of %d elements...", n));
                                     this->Layout(); });

    auto start = std::chrono::steady_clock::now();

    // uso il bubblesort che è pessimo per le performance
    for (int i = 0; i < n - 1; i++)
    {
        wxGetApp().CallAfter([this, n, i]()
                             { this->progressBar->SetValue(i * this->progressBar->GetRange() / (n - 2)); });

        if (this->quitRequested)
        {
            wxGetApp().CallAfter([this]()
                                 {
                                             this->processing = false;
                                             this->backgroundThread.join();
                                             this->quitRequested = false;
                                             this->Destroy(); 
                                 });
            return;
        }

        // dentro questo cliclo for modifico il dato;
        std::lock_guard g(dataMutex);
        DEBUG_LOG("mutex per il sort acquisito\n", logfile);
        for (int j = 0; j < n - i - 1; j++)
        {
            if (sharedData[j] > sharedData[j + 1])
            {
                std::swap(sharedData[j], sharedData[j + 1]);
            }
        }
        DEBUG_LOG("mutex per il sort rilasciato\n", logfile);
    }
    



    // questo è l'algoritmo introsort, molto più efficente
    //std::sort(sharedData.begin(), sharedData.end());

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;


    // il thead background ha finito, ora mando istruzioni al main thread
    wxGetApp().CallAfter([this, diff]()
        {
            auto frontValue = sharedData.front();
            this->SetStatusText(wxString::Format("The first number is: %f. Processing time: %.2f [ms]", frontValue, std::chrono::duration<double, std::milli>(diff).count()));
            this->Layout();

            this->backgroundThread.join();
            this->processing = false;
            this->button->Disable(); });
}

void MyFrame::OnResizeWindow(wxSizeEvent& e)
{
    std::stringstream oss;
    oss << "Frame Width: " << this->GetSize().GetWidth() << std::endl;
    oss << "Frame Height: " << this->GetSize().GetHeight() << std::endl;

    DEBUG_LOG(oss.str(), logfile);

    Refresh();
    e.Skip(); // lo propago a gri figli, quindi anche alla grid;
}

void MyFrame::OnTime(wxTimerEvent&)
{
    DEBUG_LOG("refresh time chiamato\n", logfile);
    grid->Refresh();
}

MyFrame::~MyFrame()
{
    DEBUG_LOG("sono dentro al distruttore di MyFrame\n", logfile);
    if (refreshTimer)
    {
        delete refreshTimer;
        refreshTimer = nullptr;
    }
    logfile.close();
    DEBUG_LOG("esco dal distruttore di MyFrame\n", logfile);
}