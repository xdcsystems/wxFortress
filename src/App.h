#pragma once

// froward declarations
class MainFrame;

class App : public wxApp
{
    public:
      virtual bool OnInit() wxOVERRIDE;
      
      // 3rd, and final, level exception handling: whenever an unhandled
      // exception is caught, this function is called
      virtual bool OnExceptionInMainLoop() wxOVERRIDE;

    private:
      MainFrame *m_mainFrame = nullptr;
};

wxDECLARE_APP( App );
